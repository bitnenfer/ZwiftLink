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

class FZwiftClickLink
{
public:
	~FZwiftClickLink();
	bool ConnectToDevice();
	void DisconnectFromDevice();
	bool IsConnected() const;
	bool IsPressed(EZwiftClickButton Button) const;
	bool WasPressed(EZwiftClickButton Button) const;
	bool IsFinished() const;
	uint8_t GetBatteryLevel() const;

private:
	struct FZwiftClickData* Data = nullptr;
};