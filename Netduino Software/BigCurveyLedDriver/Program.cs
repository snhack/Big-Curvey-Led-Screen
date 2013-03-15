using System;
using System.Threading;

using Microsoft.SPOT;
using Microsoft.SPOT.Hardware;

using GHIElectronics.NETMF.FEZ;
using GHIElectronics.NETMF.Hardware;

namespace BigCurveyLedDriver
{
	public class Program
	{
		public static void Main()
		{
			var rows = new OutputPort[7]; // 7 rows of LEDs
			rows[0] = new OutputPort((Cpu.Pin)FEZ_Pin.Digital.Di20, true);
			rows[1] = new OutputPort((Cpu.Pin)FEZ_Pin.Digital.Di21, true);
			rows[2] = new OutputPort((Cpu.Pin)FEZ_Pin.Digital.Di22, true);
			rows[3] = new OutputPort((Cpu.Pin)FEZ_Pin.Digital.Di23, true);
			rows[4] = new OutputPort((Cpu.Pin)FEZ_Pin.Digital.Di24, true);
			rows[5] = new OutputPort((Cpu.Pin)FEZ_Pin.Digital.Di25, true);
			rows[6] = new OutputPort((Cpu.Pin)FEZ_Pin.Digital.Di26, true);

			var sdata = new OutputPort((Cpu.Pin)FEZ_Pin.Digital.Di30, false);
			var sclock = new OutputPort((Cpu.Pin)FEZ_Pin.Digital.Di31, false); // Clocks data in on rising edge
			var latchEnable = new OutputPort((Cpu.Pin)FEZ_Pin.Digital.Di32, false); // latch on high pulse
			var outputEnable = new OutputPort((Cpu.Pin)FEZ_Pin.Digital.Di10, true); // Active low

			var pattern = new string[7];
			pattern[0] = "10010111101111000000000000000000";
			pattern[1] = "10010100101000000000000000000000";
			pattern[2] = "10010100101000000000000000000000";
			pattern[3] = "11110111101000000000000000000000";
			pattern[4] = "10010100101000000000000000000000";
			pattern[5] = "10010100101000000000000000000000";
			pattern[6] = "10010100101111000000000000000000";

			//pattern[0] = "10000000000000000000000000000000";
			//pattern[1] = "01000000000000000000000000000000";
			//pattern[2] = "00100000000000000000000000000000";
			//pattern[3] = "00010000000000000000000000000000";
			//pattern[4] = "00001000000000000000000000000000";
			//pattern[5] = "00000100000000000000000000000000";
			//pattern[6] = "00000010000000000000000000000000";


			while (true)
			{
				for (int row = 0; row < 7; row++)
				{
					// Disable all rows
					for (int j = 0; j < 7; j++)
						rows[j].Write(false);

					//for (var k = 0; k<2; k++)
					// Clock values in for each of the 32 LEDS
						for (var i = 0; i < 32; i++)
						{
							sclock.Write(false);
							if (pattern[row][31 - i] == '1')
								sdata.Write(true); // on
							else
								sdata.Write(false); // off
							sclock.Write(true);
						}

					sclock.Write(false); // needed?

					// Latch the values
					latchEnable.Write(true);
					latchEnable.Write(false);

					// Enable the row we want (each in turn 0...7)
					rows[row].Write(true);

					// Turn on the outputs for a period of time
					outputEnable.Write(false);
					Thread.Sleep(1);
					// Turn off again to clock in the next set of values
					outputEnable.Write(true);
					//Thread.Sleep(1);
				}
			}
		}
	}
}
