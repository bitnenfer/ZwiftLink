#include "BikeTrainerLink.h"

#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#include <SetupAPI.h>
#include <bluetoothleapis.h>
#include <bthledef.h>
#include <devpkey.h>
#include <combaseapi.h>
#include <atomic>
#include <mutex>
#include <stdio.h>
#include <winrt/Windows.Foundation.h>
#include <winrt/Windows.Foundation.Collections.h>
#include <winrt/Windows.Devices.Bluetooth.h>
#include <winrt/Windows.Devices.Bluetooth.Advertisement.h>
#include <winrt/Windows.Devices.Bluetooth.GenericAttributeProfile.h>
#include <winrt/Windows.Storage.Streams.h>

#include <thread>
#include <vector>

#pragma comment(lib, "windowsapp.lib")
#pragma comment(lib, "SetupAPI.lib")
#pragma comment(lib, "BluetoothApis.lib")
#pragma comment(lib, "ole32.lib")

#define GATT_CHAR_INDOOR_BIKE_DATA 0x2AD2
#define GATT_CHAR_FITNESS_MACHINE_CONTROL_POINT 0x2AD9
#define GATT_PROF_CLIENT_CHARACTERISTICS_CONFIGURATION 0x2902
#define BLE_FITNESS_MACHINE_SERVICE 0x1826

struct FBikeTrainerData
{
	BTH_LE_GATT_CHARACTERISTIC ControlPoint{};
	HANDLE DeviceHandle = nullptr;
	HANDLE BikeEventHandle = nullptr;
	HANDLE ControlEventHandle = nullptr;
	std::atomic<float> Speed = -1.0f;
	std::atomic<float> Cadence = -1.0f;
	std::atomic<int32_t> Power = -1;
	uint8_t Resistance = 0;
	bool Connected = false;
};

static double GetMillisecs()
{
	LARGE_INTEGER li;
	QueryPerformanceFrequency(&li);
	double freq = ((double)li.QuadPart) / 1000.0;
	QueryPerformanceCounter(&li);
	return (double)(((double)li.QuadPart) / freq);
}


static bool WriteDataToDevice(FBikeTrainerData* TrainerData, const uint8_t* Data, size_t DataSize)
{
	const size_t TotalSize = sizeof(BTH_LE_GATT_CHARACTERISTIC_VALUE) + DataSize;
	std::vector<uint8_t> Buffer(TotalSize, 0);
	BTH_LE_GATT_CHARACTERISTIC_VALUE* DataValue = (BTH_LE_GATT_CHARACTERISTIC_VALUE*)Buffer.data();
	DataValue->DataSize = (ULONG)DataSize;
	memcpy(DataValue->Data, Data, DataSize);
	return BluetoothGATTSetCharacteristicValue(TrainerData->DeviceHandle, &TrainerData->ControlPoint, DataValue, 0, BLUETOOTH_GATT_FLAG_NONE) >= 0;
}

FBikeTrainerLink::~FBikeTrainerLink()
{
	DisconnectFromDevice();
}

bool FBikeTrainerLink::ConnectToDevice()
{
	DisconnectFromDevice();

	Data = new FBikeTrainerData();

	HANDLE LocalDeviceHandle = INVALID_HANDLE_VALUE;
	HANDLE LocalEventHandle = INVALID_HANDLE_VALUE;
	BTH_LE_GATT_SERVICE FTMSService{};
	BTH_LE_GATT_CHARACTERISTIC BikeData{};
	BTH_LE_GATT_CHARACTERISTIC ControlPoint{};
	HDEVINFO DeviceInfoList = SetupDiGetClassDevsW(&GUID_BLUETOOTH_GATT_SERVICE_DEVICE_INTERFACE, nullptr, nullptr, DIGCF_PRESENT | DIGCF_DEVICEINTERFACE);
	if (DeviceInfoList == INVALID_HANDLE_VALUE)
	{
		return false;
	}

	SP_DEVICE_INTERFACE_DATA InterfaceData = {};
	InterfaceData.cbSize = sizeof(SP_DEVICE_INTERFACE_DATA);
	bool bKeepLooking = true;
	bool bFoundBikeData = false;
	bool bFoundControlPoint = false;

	for (DWORD Index = 0; bKeepLooking; ++Index)
	{
		if (!SetupDiEnumDeviceInterfaces(DeviceInfoList, nullptr, &GUID_BLUETOOTH_GATT_SERVICE_DEVICE_INTERFACE, Index, &InterfaceData))
		{
			if (GetLastError() == ERROR_NO_MORE_ITEMS) break;
			continue;
		}

		DWORD RequiredSize = 0;
		SetupDiGetDeviceInterfaceDetailW(DeviceInfoList, &InterfaceData, nullptr, 0, &RequiredSize, nullptr);
		if (RequiredSize == 0)
			continue;

		std::vector<uint8_t> Buffer((uint64_t)RequiredSize, 0);

		PSP_DEVICE_INTERFACE_DETAIL_DATA Detail = (PSP_DEVICE_INTERFACE_DETAIL_DATA)Buffer.data();
		Detail->cbSize = sizeof(SP_DEVICE_INTERFACE_DETAIL_DATA);

		SP_DEVINFO_DATA DevInfo = {};
		DevInfo.cbSize = sizeof(SP_DEVINFO_DATA);

		if (!SetupDiGetDeviceInterfaceDetailW(DeviceInfoList, &InterfaceData, Detail, RequiredSize, nullptr, &DevInfo))
		{
			continue;
		}

		HANDLE ServiceHandle = CreateFileW(Detail->DevicePath, GENERIC_READ | GENERIC_WRITE, FILE_SHARE_READ | FILE_SHARE_WRITE, nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);
		if (ServiceHandle == INVALID_HANDLE_VALUE)
		{
			continue;
		}

		USHORT ServiceCount = 0;
		BluetoothGATTGetServices(ServiceHandle, 0, nullptr, &ServiceCount, BLUETOOTH_GATT_FLAG_NONE);
		if (ServiceCount == 0)
		{
			CloseHandle(ServiceHandle);
			continue;
		}

		std::vector<BTH_LE_GATT_SERVICE> Services(ServiceCount, BTH_LE_GATT_SERVICE{});

		if (BluetoothGATTGetServices(ServiceHandle, ServiceCount, Services.data(), &ServiceCount, BLUETOOTH_GATT_FLAG_NONE) < 0)
		{
			CloseHandle(ServiceHandle);
			continue;
		}

		for (const BTH_LE_GATT_SERVICE& Service : Services)
		{
			if (Service.ServiceUuid.IsShortUuid && Service.ServiceUuid.Value.ShortUuid == BLE_FITNESS_MACHINE_SERVICE)
			{
				LocalDeviceHandle = ServiceHandle;
				FTMSService = Service;
				bKeepLooking = false;
				break;
			}
		}

		if (!bKeepLooking) break;

		CloseHandle(ServiceHandle);
	}

	SetupDiDestroyDeviceInfoList(DeviceInfoList);

	if (LocalDeviceHandle == INVALID_HANDLE_VALUE) return false;

	USHORT CharCount = 0;
	BluetoothGATTGetCharacteristics(LocalDeviceHandle, &FTMSService, 0, nullptr, &CharCount, BLUETOOTH_GATT_FLAG_NONE);
	if (CharCount == 0)
	{
		CloseHandle(LocalDeviceHandle);
		return false;
	}
	std::vector<BTH_LE_GATT_CHARACTERISTIC> Chars(CharCount, BTH_LE_GATT_CHARACTERISTIC{});
	if (BluetoothGATTGetCharacteristics(LocalDeviceHandle, &FTMSService, CharCount, Chars.data(), &CharCount, BLUETOOTH_GATT_FLAG_NONE) < 0)
	{
		CloseHandle(LocalDeviceHandle);
		return false;
	}

	for (const BTH_LE_GATT_CHARACTERISTIC& Char : Chars)
	{
		if (!Char.CharacteristicUuid.IsShortUuid)
			continue;

		switch (Char.CharacteristicUuid.Value.ShortUuid)
		{
		case GATT_CHAR_INDOOR_BIKE_DATA:
			BikeData = Char;
			bFoundBikeData = true;
			break;

		case GATT_CHAR_FITNESS_MACHINE_CONTROL_POINT:
			ControlPoint = Char;
			bFoundControlPoint = true;
			break;
		}
	}

	if (!bFoundBikeData || !bFoundControlPoint)
	{
		CloseHandle(LocalDeviceHandle);
		return false;
	}

	USHORT BikeDescCount = 0;
	BluetoothGATTGetDescriptors(LocalDeviceHandle, &BikeData, 0, nullptr, &BikeDescCount, BLUETOOTH_GATT_FLAG_NONE);
	if (BikeDescCount == 0)
	{
		CloseHandle(LocalDeviceHandle);
		return false;
	}

	std::vector<BTH_LE_GATT_DESCRIPTOR> BikeDescriptors(BikeDescCount, BTH_LE_GATT_DESCRIPTOR{});
	if (BluetoothGATTGetDescriptors(LocalDeviceHandle, &BikeData, BikeDescCount, BikeDescriptors.data(), &BikeDescCount, BLUETOOTH_GATT_FLAG_NONE) < 0)
	{
		CloseHandle(LocalDeviceHandle);
		return false;
	}

	bool bBikeCCCDEnabled = false;
	for (BTH_LE_GATT_DESCRIPTOR& Desc : BikeDescriptors)
	{
		if (!(Desc.DescriptorUuid.IsShortUuid && Desc.DescriptorUuid.Value.ShortUuid == GATT_PROF_CLIENT_CHARACTERISTICS_CONFIGURATION))
			continue;

		BTH_LE_GATT_DESCRIPTOR_VALUE Value = {};
		Value.DescriptorType = ClientCharacteristicConfiguration;
		Value.ClientCharacteristicConfiguration.IsSubscribeToNotification = true;
		Value.ClientCharacteristicConfiguration.IsSubscribeToIndication = false;

		if (BluetoothGATTSetDescriptorValue(LocalDeviceHandle, &Desc, &Value, BLUETOOTH_GATT_FLAG_NONE) < 0)
		{
			CloseHandle(LocalDeviceHandle);
			return false;
		}

		bBikeCCCDEnabled = true;
		break;
	}

	if (!bBikeCCCDEnabled)
	{
		CloseHandle(LocalDeviceHandle);
		return false;
	}

	USHORT ControlDescCount = 0;
	BluetoothGATTGetDescriptors(LocalDeviceHandle, &ControlPoint, 0, nullptr, &ControlDescCount, BLUETOOTH_GATT_FLAG_NONE);
	if (ControlDescCount == 0)
	{
		CloseHandle(LocalDeviceHandle);
		return false;
	}

	std::vector<BTH_LE_GATT_DESCRIPTOR> ControlDescriptors(ControlDescCount, BTH_LE_GATT_DESCRIPTOR{});
	if (BluetoothGATTGetDescriptors(LocalDeviceHandle, &ControlPoint, ControlDescCount, ControlDescriptors.data(), &ControlDescCount, BLUETOOTH_GATT_FLAG_NONE) < 0)
	{
		CloseHandle(LocalDeviceHandle);
		return false;
	}

	bool bControlCCCDEnabled = false;
	for (BTH_LE_GATT_DESCRIPTOR& Desc : ControlDescriptors)
	{
		if (!(Desc.DescriptorUuid.IsShortUuid && Desc.DescriptorUuid.Value.ShortUuid == GATT_PROF_CLIENT_CHARACTERISTICS_CONFIGURATION))
			continue;

		BTH_LE_GATT_DESCRIPTOR_VALUE Value = {};
		Value.DescriptorType = ClientCharacteristicConfiguration;
		Value.ClientCharacteristicConfiguration.IsSubscribeToNotification = false;
		Value.ClientCharacteristicConfiguration.IsSubscribeToIndication = true;

		if (BluetoothGATTSetDescriptorValue(LocalDeviceHandle, &Desc, &Value, BLUETOOTH_GATT_FLAG_NONE) < 0)
		{
			CloseHandle(LocalDeviceHandle);
			return false;
		}

		bControlCCCDEnabled = true;
		break;
	}

	if (!bControlCCCDEnabled)
	{
		CloseHandle(LocalDeviceHandle);
		return false;
	}

	BLUETOOTH_GATT_VALUE_CHANGED_EVENT_REGISTRATION BikeEventRegistration = {};
	BikeEventRegistration.NumCharacteristics = 1;
	BikeEventRegistration.Characteristics[0] = BikeData;

	auto OnBikeDataEvent = [](BTH_LE_GATT_EVENT_TYPE EventType, PVOID EventOutParameter, PVOID ThisPtr)
	{
		if (EventType != CharacteristicValueChangedEvent)
			return;

		PBLUETOOTH_GATT_VALUE_CHANGED_EVENT Event = (PBLUETOOTH_GATT_VALUE_CHANGED_EVENT)EventOutParameter;

		if (!Event || !Event->CharacteristicValue)
		{
			return;
		}

		FBikeTrainerData* This = (FBikeTrainerData*)ThisPtr;
		const uint8_t* Ptr = Event->CharacteristicValue->Data;
		const size_t Size = Event->CharacteristicValue->DataSize;
		const uint8_t* End = Ptr + Size;

		if (!(Ptr + 2 <= End)) return;

		const uint16_t Flags = (uint16_t)(Ptr[0]) | (uint16_t)(Ptr[1] << 8);
		Ptr += 2;

		const bool bMoreData = (Flags & (1 << 0)) != 0;
		const bool bAvgSpeedPresent = (Flags & (1 << 1)) != 0;
		const bool bCadencePresent = (Flags & (1 << 2)) != 0;
		const bool bAvgCadencePresent = (Flags & (1 << 3)) != 0;
		const bool bDistancePresent = (Flags & (1 << 4)) != 0;
		const bool bResistancePresent = (Flags & (1 << 5)) != 0;
		const bool bPowerPresent = (Flags & (1 << 6)) != 0;
		const bool bAvgPowerPresent = (Flags & (1 << 7)) != 0;

		if (!bMoreData)
		{
			if (!(Ptr + 2 <= End)) return;
			float Speed = (float)((uint16_t)(Ptr[0]) | (uint16_t)(Ptr[1] << 8)) * 0.01f;
			This->Speed.store(Speed);
			Ptr += 2;
		}

		if (bAvgSpeedPresent)
		{
			if (!(Ptr + 2 <= End)) return;
			Ptr += 2;
		}

		if (bCadencePresent)
		{
			if (!(Ptr + 2 <= End)) return;
			float Cadence = (float)((uint16_t)(Ptr[0]) | (uint16_t)(Ptr[1] << 8)) * 0.5f;
			This->Cadence.store(Cadence);
			Ptr += 2;
		}

		if (bAvgCadencePresent)
		{
			if (!(Ptr + 2 <= End)) return;
			Ptr += 2;
		}

		if (bDistancePresent)
		{
			if (!(Ptr + 3 <= End)) return;
			Ptr += 3;
		}

		if (bResistancePresent)
		{
			if (!(Ptr + 2 <= End)) return;
			Ptr += 2;
		}

		if (bPowerPresent)
		{
			if (!(Ptr + 2 <= End)) return;
			int16_t Power = (int16_t)((uint16_t)(Ptr[0]) | (uint16_t)(Ptr[1] << 8));
			This->Power.store(Power);
			Ptr += 2;
		}

		if (bAvgPowerPresent)
		{
			if (!(Ptr + 2 <= End)) return;
			Ptr += 2;
		}
	};

	if (BluetoothGATTRegisterEvent(LocalDeviceHandle, CharacteristicValueChangedEvent, &BikeEventRegistration, OnBikeDataEvent, Data, &LocalEventHandle, BLUETOOTH_GATT_FLAG_NONE) < 0)
	{
		CloseHandle(LocalDeviceHandle);
		return false;
	}

	HANDLE LocalControlEventHandle = INVALID_HANDLE_VALUE;
	BLUETOOTH_GATT_VALUE_CHANGED_EVENT_REGISTRATION ControlEventRegistration = {};
	ControlEventRegistration.NumCharacteristics = 1;
	ControlEventRegistration.Characteristics[0] = ControlPoint;

	auto OnControlPointEvent = [](BTH_LE_GATT_EVENT_TYPE EventType, PVOID EventOutParameter, PVOID ThisPtr)
	{
		if (EventType != CharacteristicValueChangedEvent)
			return;

		PBLUETOOTH_GATT_VALUE_CHANGED_EVENT Event = (PBLUETOOTH_GATT_VALUE_CHANGED_EVENT)EventOutParameter;

		if (!Event || !Event->CharacteristicValue)
		{
			return;
		}

		const uint8_t* Ptr = Event->CharacteristicValue->Data;
		const size_t Size = Event->CharacteristicValue->DataSize;

		if (Size < 3 || Ptr[0] != 0x80)
			return;
	};

	if (BluetoothGATTRegisterEvent(LocalDeviceHandle, CharacteristicValueChangedEvent, &ControlEventRegistration, OnControlPointEvent, Data, &LocalControlEventHandle, BLUETOOTH_GATT_FLAG_NONE) < 0)
	{
		BluetoothGATTUnregisterEvent(LocalEventHandle, BLUETOOTH_GATT_FLAG_NONE);
		CloseHandle(LocalDeviceHandle);
		return false;
	}

	Data->DeviceHandle = LocalDeviceHandle;
	Data->BikeEventHandle = LocalEventHandle;
	Data->ControlEventHandle = LocalControlEventHandle;
	Data->ControlPoint = ControlPoint;

	const uint8_t RequestControl[] = { 0 };
	if (!WriteDataToDevice(Data, RequestControl, sizeof(RequestControl)))
	{
		DisconnectFromDevice();
		return false;
	}

	Data->Connected = true;
	return true;
}

void FBikeTrainerLink::DisconnectFromDevice()
{
	if (Data)
	{
		if (Data->BikeEventHandle && Data->BikeEventHandle != INVALID_HANDLE_VALUE)
		{
			BluetoothGATTUnregisterEvent(Data->BikeEventHandle, BLUETOOTH_GATT_FLAG_NONE);
			Data->BikeEventHandle = nullptr;
		}

		if (Data->ControlEventHandle && Data->ControlEventHandle != INVALID_HANDLE_VALUE)
		{
			BluetoothGATTUnregisterEvent(Data->ControlEventHandle, BLUETOOTH_GATT_FLAG_NONE);
			Data->ControlEventHandle = nullptr;
		}

		if (Data->DeviceHandle && Data->DeviceHandle != INVALID_HANDLE_VALUE)
		{
			CloseHandle(Data->DeviceHandle);
			Data->DeviceHandle = nullptr;
		}

		delete Data;
		Data = nullptr;
	}
}

bool FBikeTrainerLink::IsConnected() const
{
	return Data && Data->Connected;
}

float FBikeTrainerLink::GetSpeed() const
{
	return Data ? Data->Speed.load() : -1.0f;
}

float FBikeTrainerLink::GetCadence() const
{
	return Data ? Data->Cadence.load() : -1.0f;
}

int32_t FBikeTrainerLink::GetPower() const
{
	return Data ? Data->Power.load() : -1;
}

uint8_t FBikeTrainerLink::GetResistance() const
{
	return Data ? Data->Resistance : 0;
}

bool FBikeTrainerLink::SetResistance(uint8_t NewResistance)
{
	if (Data && Data->Connected)
	{
		const uint8_t ResistanceCommand[] = { 4, NewResistance };
		if (WriteDataToDevice(Data, ResistanceCommand, sizeof(ResistanceCommand)))
		{
			Data->Resistance = NewResistance;
			return true;
		}
	}
	return false;
}



using namespace winrt;
using namespace winrt::Windows::Foundation;
using namespace winrt::Windows::Devices::Bluetooth;
using namespace winrt::Windows::Devices::Bluetooth::Advertisement;
using namespace winrt::Windows::Devices::Bluetooth::GenericAttributeProfile;
using namespace winrt::Windows::Storage::Streams;

static const winrt::guid ZWIFT_SERVICE_UUID{ 0x0000FC82, 0x0000, 0x1000, { 0x80, 0x00, 0x00, 0x80, 0x5F, 0x9B, 0x34, 0xFB } };
static const winrt::guid ZWIFT_ASYNC_UUID{ 0x00000002, 0x19ca, 0x4651, { 0x86, 0xe5, 0xfa, 0x29, 0xdc, 0xdd, 0x09, 0xd1 } };
static const winrt::guid ZWIFT_SYNC_RX_UUID{ 0x00000003, 0x19ca, 0x4651, { 0x86, 0xe5, 0xfa, 0x29, 0xdc, 0xdd, 0x09, 0xd1 } };
static const winrt::guid ZWIFT_SYNC_TX_UUID{ 0x00000004, 0x19ca, 0x4651, { 0x86, 0xe5, 0xfa, 0x29, 0xdc, 0xdd, 0x09, 0xd1 } };

static const winrt::guid ZWIFT_LEGACY_SERVICE_UUID{ 0x00000001, 0x19ca, 0x4651, { 0x86, 0xe5, 0xfa, 0x29, 0xdc, 0xdd, 0x09, 0xd1 } };

#define ZWIFT_NAME_FILTER        L"Zwift Click"
#define ZWIFT_SCAN_TIMEOUT_MS    15000
#define ZWIFT_HANDSHAKE_WAIT_MS  5000
#define ZWIFT_CAPABILITY_WAIT_MS 2000

#define ZWIFT_VERBOSE 0

#if ZWIFT_VERBOSE > 0
#define ZWIFT_TRACE  DBG_LOG
#else
#define ZWIFT_TRACE(...)  ((void)0)
#endif

static void EnsureWinRTApartment()
{
	static std::once_flag Once;
	std::call_once(Once, []()
	{
		CO_MTA_USAGE_COOKIE Cookie = nullptr;
		CoIncrementMTAUsage(&Cookie); 
	});
}

struct FZwiftClickData
{
	FZwiftClickData()
	{
		for (uint32_t Index = 0; Index < (uint32_t)EZwiftClickButton::COUNT; ++Index)
			Buttons[Index].store(false);
	}

	BluetoothLEDevice   Device{ nullptr };
	GattSession         Session{ nullptr };
	GattDeviceService   Service{ nullptr };
	GattCharacteristic  AsyncCharacteristic{ nullptr };
	GattCharacteristic  SyncRXCharacteristic{ nullptr };
	GattCharacteristic  SyncTXCharacteristic{ nullptr };

	GattCharacteristic::ValueChanged_revoker           AsyncRevoker;
	GattCharacteristic::ValueChanged_revoker           SyncTXRevoker;
	BluetoothLEDevice::ConnectionStatusChanged_revoker StatusRevoker;

	std::thread Worker;
	HANDLE StopEvent = nullptr;
	HANDLE HandshakeEvent = nullptr;

	std::atomic<bool> Buttons[(uint32_t)EZwiftClickButton::COUNT];
	std::atomic<uint32_t> ButtonMap{ 0xFFFFFFFFu };
	std::atomic<uint32_t> PresentMask{ 0 };
	std::atomic<uint32_t> PressedEdges{ 0 };
	std::atomic<uint32_t> PreviousPressed{ 0 };
	std::atomic<uint8_t>  BatteryLevel{ 0 };
	std::atomic<bool> Connected{ false };
	std::atomic<bool> Failed{ false };
	std::atomic<bool> ConnectionInProgress{ false };
	std::atomic<bool> RejectedForCapability{ false };

	std::atomic<uint64_t> DeviceAddress{ 0 };
	uint64_t PreferredAddress = 0;
	uint32_t RequiredMask = 0;
	uint64_t ExcludedAddresses[ZWIFT_MAX_REJECTED_DEVICES] = {};
	uint32_t ExcludedCount = 0;
};

static IBuffer MakeBuffer(const uint8_t* Bytes, size_t Size)
{
	DataWriter Writer;
	Writer.WriteBytes(winrt::array_view<const uint8_t>(Bytes, Bytes + Size));
	return Writer.DetachBuffer();
}

static void BufferToBytes(const IBuffer& Buffer, std::vector<uint8_t>& OutBytes)
{
	DataReader Reader = DataReader::FromBuffer(Buffer);
	OutBytes.resize(Reader.UnconsumedBufferLength());
	if (!OutBytes.empty())
		Reader.ReadBytes(OutBytes);
}

static void LogBytes(const char* Prefix, const uint8_t* Ptr, size_t Size)
{
	char Debug[256]{};
	size_t Offset = 0;

	for (size_t Index = 0; Index < Size; ++Index)
	{
		if (Offset + 4 >= sizeof(Debug))
			break;

		int Written = snprintf(Debug + Offset, sizeof(Debug) - Offset, "%02X ", Ptr[Index]);
		if (Written <= 0)
			break;

		Offset += (size_t)Written;
	}

	ZWIFT_TRACE("%s: %s", Prefix, Debug);
}

static bool WriteZwiftData(FZwiftClickData* ClickData, const uint8_t* Bytes, size_t Size)
{
	if (!ClickData || !ClickData->SyncRXCharacteristic)
		return false;

	const GattCharacteristicProperties Props = ClickData->SyncRXCharacteristic.CharacteristicProperties();
	const GattWriteOption Option =
		((Props & GattCharacteristicProperties::Write) != GattCharacteristicProperties::None)
		? GattWriteOption::WriteWithResponse
		: GattWriteOption::WriteWithoutResponse;

	try
	{
		return ClickData->SyncRXCharacteristic.WriteValueAsync(MakeBuffer(Bytes, Size), Option).get()
			== GattCommunicationStatus::Success;
	}
#if ZWIFT_VERBOSE
	catch (const winrt::hresult_error& Error)
	{
		ZWIFT_TRACE("ZWIFT: write failed 0x%08X", (uint32_t)Error.code());
		return false;
	}
#else
	catch (const winrt::hresult_error&)
	{
		return false;

	}
#endif
}

static bool ReadVarint(const uint8_t*& Ptr, const uint8_t* End, uint64_t& OutValue)
{
	OutValue = 0;
	uint32_t Shift = 0;

	while (Ptr < End && Shift < 64)
	{
		const uint8_t Byte = *Ptr++;
		OutValue |= (uint64_t)(Byte & 0x7F) << Shift;

		if ((Byte & 0x80) == 0)
			return true;

		Shift += 7;
	}
	return false;
}

static bool ReadVarintField(const uint8_t* Ptr, const uint8_t* End, uint32_t WantField, uint64_t& OutValue)
{
	while (Ptr < End)
	{
		uint64_t Tag = 0;
		if (!ReadVarint(Ptr, End, Tag))
			return false;

		const uint32_t Field = (uint32_t)(Tag >> 3);
		const uint32_t Wire = (uint32_t)(Tag & 0x7);

		if (Field == WantField && Wire == 0)
			return ReadVarint(Ptr, End, OutValue);

		switch (Wire)
		{
		case 0: { uint64_t Skip = 0; if (!ReadVarint(Ptr, End, Skip)) return false; break; }
		case 1: { if (End - Ptr < 8) return false; Ptr += 8; break; }
		case 2:
		{
			uint64_t Length = 0;
			if (!ReadVarint(Ptr, End, Length)) return false;
			if ((uint64_t)(End - Ptr) < Length) return false;
			Ptr += (size_t)Length;
			break;
		}
		case 5: { if (End - Ptr < 4) return false; Ptr += 4; break; }
		default: return false;
		}
	}
	return false;
}

static constexpr uint32_t ZwiftClickButtonMasks[(uint32_t)EZwiftClickButton::COUNT] =
{
	0x00000001,
	0x00000002,
	0x00000004,
	0x00000008,
	0x00000100,
	0x00000010,
	0x00000020,
	0x00000040,
	0x00000080,
	0x00001000,
};
static_assert(sizeof(ZwiftClickButtonMasks) / sizeof(ZwiftClickButtonMasks[0])
	== (size_t)EZwiftClickButton::COUNT, "button mask table out of sync with enum");

uint32_t ZwiftButtonMask(EZwiftClickButton Button)
{
	if (Button >= EZwiftClickButton::COUNT)
		return 0;

	return ZwiftClickButtonMasks[(uint8_t)Button];
}

uint32_t ZwiftAllButtonsMask()
{
	uint32_t Mask = 0;
	for (uint32_t Index = 0; Index < (uint32_t)EZwiftClickButton::COUNT; ++Index)
		Mask |= ZwiftClickButtonMasks[Index];

	return Mask;
}

static void ApplyButtonMap(FZwiftClickData* ClickData, uint32_t Map)
{
	const uint32_t Present = ClickData->PresentMask.fetch_or(Map) | Map;

	ClickData->ButtonMap.store(Map);

	const uint32_t Pressed = ~Map & Present;

	for (uint32_t Index = 0; Index < (uint32_t)EZwiftClickButton::COUNT; ++Index)
		ClickData->Buttons[Index].store((Pressed & ZwiftClickButtonMasks[Index]) != 0);

	const uint32_t Previous = ClickData->PreviousPressed.exchange(Pressed);
	ClickData->PressedEdges.fetch_or(Pressed & ~Previous);
}

static void OnAsyncValueChanged(FZwiftClickData* ClickData, const IBuffer& Value)
{
	std::vector<uint8_t> Bytes;
	BufferToBytes(Value, Bytes);
	if (Bytes.empty())
		return;

	const uint8_t* Ptr = Bytes.data();
	const size_t Size = Bytes.size();
	const unsigned long long Addr = (unsigned long long)ClickData->DeviceAddress.load();

	switch (Ptr[0])
	{
	case 0x23:
	{
		uint64_t Map = 0xFFFFFFFFull;
		if (ReadVarintField(Ptr + 1, Ptr + Size, 1, Map))
		{
			ZWIFT_TRACE("ZWIFT(%012llX): button map 0x%08X", Addr, (uint32_t)Map);
			ApplyButtonMap(ClickData, (uint32_t)Map);
		}
		else
		{
			LogBytes("ZWIFT: unparsed 0x23", Ptr, Size);
		}
		break;
	}

	case 0x19:
	{
		uint64_t Percent = 0;
		if (ReadVarintField(Ptr + 1, Ptr + Size, 2, Percent) && Percent <= 100)
		{
			ClickData->BatteryLevel.store((uint8_t)Percent);
			ZWIFT_TRACE("ZWIFT(%012llX): battery %u%%", Addr, (uint32_t)Percent);
		}
		break;
	}

	case 0x15:
	case 0x2A:
		ZWIFT_TRACE("ZWIFT(%012llX): status 0x%02X", Addr, Ptr[0]);
		break;

	default:
		if (Size >= 3 && Ptr[0] == 0xFF && Ptr[1] == 0x05 && Ptr[2] == 0x00)
		{
			ZWIFT_TRACE("ZWIFT(%012llX): device notice 0x%02X%02X", Addr,
				Size > 3 ? Ptr[3] : 0, Size > 4 ? Ptr[4] : 0);
		}
		else
		{
			ZWIFT_TRACE("ZWIFT(%012llX) ASYNC (unknown id 0x%02X)", Addr, Ptr[0]);
		}
		break;
	}
}

static void OnSyncTXValueChanged(FZwiftClickData* ClickData, const IBuffer& Value)
{
	std::vector<uint8_t> Bytes;
	BufferToBytes(Value, Bytes);
	if (Bytes.empty())
		return;

	const unsigned long long Addr = (unsigned long long)ClickData->DeviceAddress.load();

	ZWIFT_TRACE("ZWIFT(%012llX) SYNC TX (%zu bytes)", Addr, Bytes.size());

	if (Bytes.size() >= 6 && memcmp(Bytes.data(), "RideOn", 6) == 0)
	{
		if (Bytes.size() >= 8)
		{
			ZWIFT_TRACE("ZWIFT(%012llX): handshake ok (version=%u flags=%u)", Addr, Bytes[6], Bytes[7]);
		}
		else
		{
			ZWIFT_TRACE("ZWIFT(%012llX): handshake ok (%zu bytes)", Addr, Bytes.size());
		}

		if (ClickData->HandshakeEvent)
			SetEvent(ClickData->HandshakeEvent);
	}
}

struct FScanState
{
	std::atomic<uint64_t> Address{ 0 };
	std::atomic<int32_t> AddressType{ (int32_t)BluetoothAddressType::Unspecified };
	HANDLE FoundEvent = nullptr;
};

static uint64_t ScanForDevice(HANDLE StopEvent, uint32_t TimeoutMs, BluetoothAddressType& OutType,
	uint64_t PreferredAddress, const uint64_t* Excluded, uint32_t ExcludedCount)
{
	OutType = BluetoothAddressType::Unspecified;

	FScanState State;
	State.FoundEvent = CreateEventW(nullptr, TRUE, FALSE, nullptr);
	if (!State.FoundEvent)
		return 0;

	BluetoothLEAdvertisementWatcher Watcher;
	Watcher.ScanningMode(BluetoothLEScanningMode::Active);

	auto Token = Watcher.Received([&State, PreferredAddress, Excluded, ExcludedCount](
		BluetoothLEAdvertisementWatcher const&,
		BluetoothLEAdvertisementReceivedEventArgs const& Args)
	{
		if (State.Address.load() != 0)
			return;

		const uint64_t Addr = Args.BluetoothAddress();

		if (PreferredAddress != 0 && Addr != PreferredAddress)
			return;

		for (uint32_t Index = 0; Index < ExcludedCount; ++Index)
		{
			if (Addr == Excluded[Index])
				return;
		}

		const auto Name = Args.Advertisement().LocalName();

		ZWIFT_TRACE("ADV %012llX rssi=%d type=%d name=%ls",
			(unsigned long long)Addr,
			(int)Args.RawSignalStrengthInDBm(),
			(int)Args.BluetoothAddressType(),
			Name.c_str());

		bool bMatch = !Name.empty() && wcsstr(Name.c_str(), ZWIFT_NAME_FILTER) != nullptr;

		if (!bMatch)
		{
			for (auto const& Uuid : Args.Advertisement().ServiceUuids())
			{
				if (Uuid == ZWIFT_SERVICE_UUID || Uuid == ZWIFT_LEGACY_SERVICE_UUID)
				{
					bMatch = true;
					break;
				}
			}
		}

		if (bMatch)
		{
			uint64_t Expected = 0;
			if (State.Address.compare_exchange_strong(Expected, Addr))
			{
				State.AddressType.store((int32_t)Args.BluetoothAddressType());
				SetEvent(State.FoundEvent);
			}
		}
	});

	Watcher.Start();

	HANDLE WaitHandles[2] = { State.FoundEvent, StopEvent };
	WaitForMultipleObjects(2, WaitHandles, FALSE, TimeoutMs);

	Watcher.Received(Token);
	Watcher.Stop();
	CloseHandle(State.FoundEvent);

	OutType = (BluetoothAddressType)State.AddressType.load();
	return State.Address.load();
}

static bool FindService(FZwiftClickData* ClickData)
{
	const winrt::guid Candidates[] = { ZWIFT_SERVICE_UUID, ZWIFT_LEGACY_SERVICE_UUID };

	for (int Attempt = 0; Attempt < 3; ++Attempt)
	{
		for (const winrt::guid& Uuid : Candidates)
		{
			auto Result = ClickData->Device.GetGattServicesForUuidAsync(Uuid, BluetoothCacheMode::Uncached).get();

			if (Result.Status() == GattCommunicationStatus::Success && Result.Services().Size() > 0)
			{
				ClickData->Service = Result.Services().GetAt(0);
				return true;
			}
		}

		ZWIFT_TRACE("ZWIFT: service not up yet (attempt %d, connstatus=%d)", Attempt, (int)ClickData->Device.ConnectionStatus());
		Sleep(250);
	}

	return false;
}

static bool SetupConnection(FZwiftClickData* ClickData, uint64_t Address, BluetoothAddressType AddressType)
{
	ClickData->Device = BluetoothLEDevice::FromBluetoothAddressAsync(Address, AddressType).get();
	if (!ClickData->Device)
	{
		ZWIFT_TRACE("ZWIFT: FromBluetoothAddressAsync returned null");
		return false;
	}

	ClickData->Session = GattSession::FromDeviceIdAsync(ClickData->Device.BluetoothDeviceId()).get();
	ClickData->Session.MaintainConnection(true);

	if (!FindService(ClickData))
	{
		ZWIFT_TRACE("ZWIFT: no Zwift service on device");
		return false;
	}

	auto GetChar = [&](const winrt::guid& Uuid, GattCharacteristic& Out) -> bool
	{
		auto Result = ClickData->Service.GetCharacteristicsForUuidAsync(Uuid, BluetoothCacheMode::Uncached).get();
		if (Result.Status() != GattCommunicationStatus::Success || Result.Characteristics().Size() == 0)
			return false;
		Out = Result.Characteristics().GetAt(0);
		return true;
	};

	if (!GetChar(ZWIFT_ASYNC_UUID, ClickData->AsyncCharacteristic) ||
		!GetChar(ZWIFT_SYNC_RX_UUID, ClickData->SyncRXCharacteristic) ||
		!GetChar(ZWIFT_SYNC_TX_UUID, ClickData->SyncTXCharacteristic))
	{
		ZWIFT_TRACE("ZWIFT: missing characteristics");
		return false;
	}

	ClickData->StatusRevoker = ClickData->Device.ConnectionStatusChanged(winrt::auto_revoke,
		[ClickData](BluetoothLEDevice const& Sender, IInspectable const&)
	{
		const bool bUp = Sender.ConnectionStatus() == BluetoothConnectionStatus::Connected;
		ZWIFT_TRACE("ZWIFT: link %s", bUp ? "up" : "down");

		if (!bUp)
		{
			ClickData->Connected.store(false);
			ClickData->Failed.store(true);

			for (uint32_t Index = 0; Index < (uint32_t)EZwiftClickButton::COUNT; ++Index)
			{
				ClickData->Buttons[Index].store(false);
			}

			ClickData->PressedEdges.store(0);
			ClickData->PreviousPressed.store(0);
		}
	});

	ClickData->SyncTXRevoker = ClickData->SyncTXCharacteristic.ValueChanged(winrt::auto_revoke,
		[ClickData](GattCharacteristic const&, GattValueChangedEventArgs const& Args)
	{
		OnSyncTXValueChanged(ClickData, Args.CharacteristicValue());
	});

	if (ClickData->SyncTXCharacteristic.WriteClientCharacteristicConfigurationDescriptorAsync(
		GattClientCharacteristicConfigurationDescriptorValue::Indicate).get() != GattCommunicationStatus::Success)
	{
		ZWIFT_TRACE("ZWIFT: failed to subscribe to syncTX indications");
		return false;
	}

	ResetEvent(ClickData->HandshakeEvent);

	const uint8_t Handshake[] = { 'R', 'i', 'd', 'e', 'O', 'n' };
	if (!WriteZwiftData(ClickData, Handshake, sizeof(Handshake)))
	{
		ZWIFT_TRACE("ZWIFT: handshake write failed");
		return false;
	}

	HANDLE HandshakeWait[2] = { ClickData->HandshakeEvent, ClickData->StopEvent };
	if (WaitForMultipleObjects(2, HandshakeWait, FALSE, ZWIFT_HANDSHAKE_WAIT_MS) != WAIT_OBJECT_0)
	{
		ZWIFT_TRACE("ZWIFT: no handshake response");
		return false;
	}

	ClickData->AsyncRevoker = ClickData->AsyncCharacteristic.ValueChanged(winrt::auto_revoke,
		[ClickData](GattCharacteristic const&, GattValueChangedEventArgs const& Args)
	{
		OnAsyncValueChanged(ClickData, Args.CharacteristicValue());
	});

	if (ClickData->AsyncCharacteristic.WriteClientCharacteristicConfigurationDescriptorAsync(
		GattClientCharacteristicConfigurationDescriptorValue::Notify).get() != GattCommunicationStatus::Success)
	{
		ZWIFT_TRACE("ZWIFT: failed to subscribe to async notifications");
		return false;
	}

	const uint8_t ClickV2Setup[] = { 0xFF, 0x04, 0x00 };
	WriteZwiftData(ClickData, ClickV2Setup, sizeof(ClickV2Setup));

	if (ClickData->RequiredMask != 0)
	{
		const uint32_t Attempts = ZWIFT_CAPABILITY_WAIT_MS / 100;

		for (uint32_t Attempt = 0; Attempt < Attempts; ++Attempt)
		{
			const uint32_t Present = ClickData->PresentMask.load();
			if ((Present & ClickData->RequiredMask) == ClickData->RequiredMask)
				return true;

			if (WaitForSingleObject(ClickData->StopEvent, 100) == WAIT_OBJECT_0)
				return false;
		}

		ZWIFT_TRACE("ZWIFT: %012llX lacks required buttons (has 0x%08X, need 0x%08X) -- skipping it",
			(unsigned long long)Address,
			ClickData->PresentMask.load(),
			ClickData->RequiredMask);

		ClickData->RejectedForCapability.store(true);
		return false;
	}

	return true;
}

static void TearDownConnection(FZwiftClickData* ClickData)
{
	ClickData->AsyncRevoker.revoke();
	ClickData->SyncTXRevoker.revoke();
	ClickData->StatusRevoker.revoke();

	ClickData->AsyncCharacteristic = nullptr;
	ClickData->SyncRXCharacteristic = nullptr;
	ClickData->SyncTXCharacteristic = nullptr;

	if (ClickData->Service) { ClickData->Service.Close(); ClickData->Service = nullptr; }
	if (ClickData->Session) { ClickData->Session.Close(); ClickData->Session = nullptr; }
	if (ClickData->Device) { ClickData->Device.Close();  ClickData->Device = nullptr; }
}

static void ZwiftWorkerMain(FZwiftClickData* ClickData)
{
	EnsureWinRTApartment();

	try
	{
		winrt::init_apartment(winrt::apartment_type::multi_threaded);
	}
	catch (const winrt::hresult_error&)
	{
	}

	try
	{
		BluetoothAddressType AddressType = BluetoothAddressType::Unspecified;
		const uint64_t Address = ScanForDevice(ClickData->StopEvent, ZWIFT_SCAN_TIMEOUT_MS, AddressType,
			ClickData->PreferredAddress, ClickData->ExcludedAddresses, ClickData->ExcludedCount);

		if (Address == 0)
		{
			ZWIFT_TRACE("ZWIFT: no device found while scanning");
			ClickData->Failed.store(true);
		}
		else
		{
			ClickData->DeviceAddress.store(Address);

			ZWIFT_TRACE("ZWIFT: connecting to %012llX (addrtype=%d)", (unsigned long long)Address, (int)AddressType);

			if (SetupConnection(ClickData, Address, AddressType))
			{
				ZWIFT_TRACE("ZWIFT: connected %012llX (buttons 0x%08X)", (unsigned long long)Address, ClickData->PresentMask.load());
				ClickData->Connected.store(true);
			}
			else
			{
				ClickData->Failed.store(true);
			}
		}
	}
#if ZWIFT_VERBOSE
	catch (const winrt::hresult_error& Error)
	{
		ZWIFT_TRACE("ZWIFT: exception 0x%08X", (uint32_t)Error.code());
		ClickData->Failed.store(true);
	}
#else
	catch (const winrt::hresult_error&) {}
#endif

	ClickData->ConnectionInProgress.store(false);

	WaitForSingleObject(ClickData->StopEvent, INFINITE);

	ClickData->Connected.store(false);
	TearDownConnection(ClickData);
}

FZwiftClickLink::~FZwiftClickLink()
{
	DisconnectFromDevice();
}

bool FZwiftClickLink::ConnectToDevice(uint64_t PreferredAddress, uint32_t RequiredButtons)
{
	DisconnectFromDevice();

	if (PreferredAddress == 0)
		PreferredAddress = LastAddress;

	for (uint32_t Index = 0; Index < RejectedCount; ++Index)
	{
		if (PreferredAddress == RejectedAddresses[Index])
		{
			PreferredAddress = 0;
			break;
		}
	}

	Data = new FZwiftClickData();
	Data->PreferredAddress = PreferredAddress;
	Data->RequiredMask = RequiredButtons;
	Data->ExcludedCount = RejectedCount;

	for (uint32_t Index = 0; Index < RejectedCount; ++Index)
		Data->ExcludedAddresses[Index] = RejectedAddresses[Index];

	Data->StopEvent = CreateEventW(nullptr, TRUE, FALSE, nullptr);
	Data->HandshakeEvent = CreateEventW(nullptr, TRUE, FALSE, nullptr);

	if (!Data->StopEvent || !Data->HandshakeEvent)
	{
		if (Data->StopEvent) CloseHandle(Data->StopEvent);
		if (Data->HandshakeEvent) CloseHandle(Data->HandshakeEvent);
		delete Data;
		Data = nullptr;
		return false;
	}

	Data->ConnectionInProgress.store(true);
	Data->Worker = std::thread(ZwiftWorkerMain, Data);
	return true;
}

void FZwiftClickLink::DisconnectFromDevice()
{
	if (!Data)
		return;

	const uint64_t Address = Data->DeviceAddress.load();

	if (Address != 0)
	{
		if (Data->RejectedForCapability.load())
		{
			bool bAlreadyListed = false;
			for (uint32_t Index = 0; Index < RejectedCount; ++Index)
			{
				if (RejectedAddresses[Index] == Address)
				{
					bAlreadyListed = true;
					break;
				}
			}

			if (!bAlreadyListed && RejectedCount < ZWIFT_MAX_REJECTED_DEVICES)
				RejectedAddresses[RejectedCount++] = Address;

			if (LastAddress == Address)
				LastAddress = 0;
		}
		else
		{
			LastAddress = Address;
		}
	}

	if (Data->StopEvent)
		SetEvent(Data->StopEvent);

	if (Data->Worker.joinable())
		Data->Worker.join();

	if (Data->StopEvent)
	{
		CloseHandle(Data->StopEvent);
		Data->StopEvent = nullptr;
	}

	if (Data->HandshakeEvent)
	{
		CloseHandle(Data->HandshakeEvent);
		Data->HandshakeEvent = nullptr;
	}

	delete Data;
	Data = nullptr;
}

void FZwiftClickLink::ForgetRejections()
{
	RejectedCount = 0;
	for (uint32_t Index = 0; Index < ZWIFT_MAX_REJECTED_DEVICES; ++Index)
		RejectedAddresses[Index] = 0;
}

bool FZwiftClickLink::IsConnected() const
{
	return Data && Data->Connected.load();
}

bool FZwiftClickLink::IsConnectionInProgress() const
{
	return Data && Data->ConnectionInProgress.load();
}

bool FZwiftClickLink::IsFinished() const
{
	return Data && (Data->Connected.load() || Data->Failed.load());
}

uint8_t FZwiftClickLink::GetBatteryLevel() const
{
	return Data ? Data->BatteryLevel.load() : 0;
}

uint64_t FZwiftClickLink::GetDeviceAddress() const
{
	if (Data)
	{
		const uint64_t Address = Data->DeviceAddress.load();
		if (Address != 0)
			return Address;
	}
	return LastAddress;
}

uint32_t FZwiftClickLink::GetPresentButtons() const
{
	return Data ? Data->PresentMask.load() : 0;
}

bool FZwiftClickLink::HasButton(EZwiftClickButton Button) const
{
	if (!Data || Button >= EZwiftClickButton::COUNT)
		return false;

	return (Data->PresentMask.load() & ZwiftClickButtonMasks[(uint8_t)Button]) != 0;
}

bool FZwiftClickLink::IsPressed(EZwiftClickButton Button) const
{
	if (Data && Data->Connected.load() && Button < EZwiftClickButton::COUNT)
		return Data->Buttons[(uint8_t)Button].load();

	return false;
}

bool FZwiftClickLink::WasPressed(EZwiftClickButton Button) const
{
	if (!Data || !Data->Connected.load() || Button >= EZwiftClickButton::COUNT)
		return false;

	const uint32_t Mask = ZwiftClickButtonMasks[(uint8_t)Button];
	return (Data->PressedEdges.fetch_and(~Mask) & Mask) != 0;
}

bool FBikeLink::Connect()
{
	bool bAny = false;

	if (!Trainer.IsConnected() && !bTrainerAttempted)
	{
		bTrainerAttempted = true;
		bAny |= Trainer.ConnectToDevice();
	}

	if (Click.IsConnected())
	{
		ClickFailureCount = 0;
		return bAny;
	}

	if (Click.IsConnectionInProgress())
		return bAny;

	const double Now = GetMillisecs();
	if (Now < NextClickAttempt)
		return bAny;

	NextClickAttempt = Now + 30.0;

	uint32_t Required = 0;

	if (bRequireDirectionalPad)
	{
		if (ClickFailureCount < 3)
		{
			Required =
				ZwiftButtonMask(EZwiftClickButton::LEFT) |
				ZwiftButtonMask(EZwiftClickButton::UP) |
				ZwiftButtonMask(EZwiftClickButton::RIGHT) |
				ZwiftButtonMask(EZwiftClickButton::DOWN);
		}
		else if (ClickFailureCount == 3)
		{
			ZWIFT_TRACE("ZWIFT: no controller with a D-pad found, accepting any device");
			Click.ForgetRejections();
		}
	}

	++ClickFailureCount;
	bAny |= Click.ConnectToDevice(0, Required);

	return bAny;
}

void FBikeLink::Disconnect()
{
	Trainer.DisconnectFromDevice();
	Click.DisconnectFromDevice();
}

float FBikeLink::GetSpeed() const
{
	return Trainer.GetSpeed();
}

int32_t FBikeLink::GetPower() const
{
	return Trainer.GetPower();
}

float FBikeLink::GetCadence() const
{
	return Trainer.GetCadence();
}

void FBikeLink::SetResistance(uint8_t Value)
{
	Trainer.SetResistance(Value);
}

bool FBikeLink::IsTrainerConnected() const
{
	return Trainer.IsConnected();
}

bool FBikeLink::IsConnected() const
{
	return IsTrainerConnected() || IsClickConnected();
}

bool FBikeLink::WasPressed(EZwiftClickButton Button) const
{
	return Click.WasPressed(Button);
}

bool FBikeLink::IsPressed(EZwiftClickButton Button) const
{
	return Click.IsPressed(Button);
}

bool FBikeLink::IsClickConnected() const
{
	return Click.IsConnected();
}

bool FBikeLink::IsConnectionInProgress() const
{
	return Click.IsConnectionInProgress();
}

uint8_t FBikeLink::GetBatteryLevel() const
{
	return Click.GetBatteryLevel();
}

void FBikeLink::Update()
{
	Connect();
}