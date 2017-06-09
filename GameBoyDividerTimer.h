#pragma once

#include "GameBoyDefs.h"

namespace GameBoy
{

	/*
	������ �������� �� ������� 16384 Hz. �� ������� ����������. ���� ����� ������ 256 ������
	*/
	class DividerTimer
	{
	public:
		DividerTimer()
		{
			Reset();
		}

		void Step(DWORD clockDelta)
		{
			ClockCounter += clockDelta;

			//DIV ������������� ������ 256 ������
			if (ClockCounter > 255)
			{
				int passedPeriods = ClockCounter / 256;
				ClockCounter %= 256;
				DIV += passedPeriods;
			}
		}

		void Reset()
		{
			ClockCounter = 0;
			DIV = 0;
		}

		void EmulateBIOS()
		{
			Reset();
		}

		void DIVChanged(BYTE value)
		{
			DIV = 0;
		}

		BYTE GetDIV()
		{
			return DIV;
		}

	private:
		DWORD ClockCounter;	//������� ������

		BYTE DIV;	
		/*
		������� �������� ������� DIV 
		�����: FF04h. 
		���: ������/������. 
		���������� ��������. 
		���������� �������� ������������� �� 1 � �������� 16384 ��.
		������ ������ �������� �� ����� ������ �������� � ��������� � ������� �������� 00 
		*/
	};

}