#pragma once

#include <stdint.h>

class FBikeTrainerLink
{
public:
	~FBikeTrainerLink();

	bool ConnectToDevice();
	void DisconnectFromDevice();
	bool IsConnected() const;
	float GetSpeed() const;
	float GetCadence() const;
	int32_t GetPower() const;
	uint8_t GetResistance() const;
	bool SetResistance(uint8_t NewResistance);

private:
	struct FBikeTrainerData* Data = nullptr;
};

enum class EZwiftClickButton : uint8_t
{
	LEFT,
	UP,
	RIGHT,
	DOWN,
	MINUS,
	A,
	B,
	Y,
	Z,
	PLUS,
	COUNT
};

#define ZWIFT_MAX_REJECTED_DEVICES 8

class FZwiftClickLink
{
public:
	~FZwiftClickLink();
	bool ConnectToDevice(uint64_t PreferredAddress = 0, uint32_t RequiredButtons = 0);
	void DisconnectFromDevice();
	void ForgetRejections();
	bool IsConnected() const;
	bool IsConnectionInProgress() const;
	bool IsFinished() const;
	bool IsPressed(EZwiftClickButton Button) const;
	bool WasPressed(EZwiftClickButton Button) const;
	bool HasButton(EZwiftClickButton Button) const;
	uint32_t GetPresentButtons() const;
	uint8_t GetBatteryLevel() const;
	uint64_t GetDeviceAddress() const;

private:
	struct FZwiftClickData* Data = nullptr;
	uint64_t LastAddress = 0;
	uint64_t RejectedAddresses[ZWIFT_MAX_REJECTED_DEVICES] = {};
	uint32_t RejectedCount = 0;
};

class FBikeLink
{
public:
	bool Connect();
	void Disconnect();
	void Update();
	float GetSpeed() const;
	int32_t GetPower() const;
	float GetCadence() const;
	void SetResistance(uint8_t Value);
	bool IsPressed(EZwiftClickButton Button) const;
	bool WasPressed(EZwiftClickButton Button) const;
	bool IsConnected() const;
	bool IsTrainerConnected() const;
	bool IsClickConnected() const;
	bool IsConnectionInProgress() const;
	uint8_t GetBatteryLevel() const;
	void SetRequireDirectionalPad(bool bRequire) { bRequireDirectionalPad = bRequire; }

private:
	FBikeTrainerLink Trainer;
	FZwiftClickLink Click;
	bool bTrainerAttempted = false;
	bool bRequireDirectionalPad = true;
	uint32_t ClickFailureCount = 0;
	double NextClickAttempt = 0.0;
};