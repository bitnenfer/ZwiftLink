#include "bodoq_engine/debug.h"
#include "bodoq_engine/time.h"
#include "BikeTrainerLink.h"

int main()
{
    //FBikeTrainerLink bikeTrainerLink{};
    //if (bikeTrainerLink.ConnectToDevice())
    //{
    //    DBG_LOG("Connected to device!");
    //    double timer = time::ms();
    //    bool check = true;

    //    while (true)
    //    {
    //        if (time::ms() - timer > 5000)
    //        {
    //            bikeTrainerLink.SetResistance(check ? 255 : 0);
    //            check = !check;
    //            timer = time::ms();
    //        }
    //        DBG_LOG("Speed %.2f km/h - Cadence %.2f rpm - Power %d W", bikeTrainerLink.GetSpeed(), bikeTrainerLink.GetCadence(), bikeTrainerLink.GetPower());
    //    }

    //    bikeTrainerLink.DisconnectFromDevice();
    //}
    //else
    //{
    //    DBG_LOG("Failed to connect");
    //}
    //return 0;

    FZwiftClickLink zwiftClickLink{};
    if (zwiftClickLink.ConnectToDevice())
    {
        DBG_LOG("Connecting to Zwift Click...");
        while (true) 
        {
            if (zwiftClickLink.IsConnected())
            {
                const char* ButtonNames[] =
                {
                    "LEFT",
                    "UP",
                    "RIGHT",
                    "DOWN",
                    "MINUS",
                    "A",
                    "B",
                    "Y",
                    "Z",
                    "PLUS"
                };

                for (uint8_t index = 0; index < (uint8_t)EZwiftClickButton::COUNT; ++index)
                {
                    if (zwiftClickLink.IsPressed((EZwiftClickButton)index))
                    {
                        DBG_LOG("Pressed %s", ButtonNames[index]);
                    }
                }
            }
        }

        zwiftClickLink.DisconnectFromDevice();
    }
    else
    {
        DBG_LOG("Failed to connect to Zwift Click");
    }

    return 0;
}