#include "GameBoyCPU.h"
#include <stdio.h>
#include <assert.h>
#include <Windows.h>

#include "GameBoyMemory.h"
#include "GameBoyInterrupts.h"
#include "GameBoyDividerTimer.h"
#include "GameBoyGPU.h"
#include "GameBoyJoypad.h"

//Синхронизация с CPU всех составляющих
//Практически все действия по 4 такта
#define SYNC_WITH_CPU(clockDelta)			\
	{DWORD adjustedClockDelta = clockDelta;	\
	GPU.Step(adjustedClockDelta, MMU);		\
	DIV.Step(adjustedClockDelta);			\
	Joypad.Step(INT);}

//Замена выражений с побитовыми операциями
#define EQUALS_ZERO(value) ((~((value) | (~(value) + 1)) >> 31) & 0x1)
#define ARE_EQUAL(A, B) ((~(((A) - (B)) | ((B) - (A))) >> 31) & 0x1)
#define IS_NEGATIVE(value) (((value) >> 31) & 0x1)
#define A_SMALLER_THAN_B(A, B) (IS_NEGATIVE((A) - (B)))

#define IS_CARRY4(value) (((value) >> 4) & 0x1)
#define IS_CARRY8(value) (((value) >> 8) & 0x1)
#define IS_CARRY12(value) (((value) >> 12) & 0x1)
#define IS_CARRY16(value) (((value) >> 16) & 0x1)

//Zero flag (7 bit) - устанавливается, когда результат операции 0 или 2 величины совпадают
#define SET_FLAG_Z(value) (F = ((value) << 7) | (F & 0x7F))

//Substract flag (6 bit) - устанавливается, если вычитание было выполнено последним
#define SET_FLAG_N(value) (F = ((value) << 6) | (F & 0xBF))

//Half carry flag (5 bit) - устанавливается, если произошел перенос из младшего полубайта
#define SET_FLAG_H(value) (F = ((value) << 5) | (F & 0xDF))

//Carry flag (4 bit) - устанавливается, если произошел перенос или в регистре A меньшее значение при выполнении инструкции CP
#define SET_FLAG_C(value) (F = ((value) << 4) | (F & 0xEF))

#define GET_FLAG_Z() ((F >> 7) & 0x1)
#define GET_FLAG_N() ((F >> 6) & 0x1)
#define GET_FLAG_H() ((F >> 5) & 0x1)
#define GET_FLAG_C() ((F >> 4) & 0x1)

//Задержка прыжка
#define DELAYED_JUMP(value)	\
	{PC = (value);			\
	SYNC_WITH_CPU(4); }

//Прибавляем к регистру A
//Затрагиваются флаги 
//Z-устанавливается, если результат 0
//N-сброс
//H-утановка, если произошел перенос из 3 бита(полубайта)
//C-установка, если произошел перенос из 7 бита(байта)
#define ADD_N(value)										\
	{SET_FLAG_Z(EQUALS_ZERO((A + (value)) & 0xFF));			\
	SET_FLAG_N(0);											\
	SET_FLAG_H(IS_CARRY4((A & 0xF) + ((value)& 0xF)));		\
	SET_FLAG_C(IS_CARRY8(A + (value)));						\
	A += (value); }

//A + n + флаг переноса
//Флаги затрагиваются:
//Z-устанавливается, если результат 0
//N-сброс
//H-устанавливается, если произошел перенос из 3 бита(полубайта)
//C-устанавливается, если произошел перенос из 7 бита(байта)
#define ADC_N(value)												\
	{int FlagC = GET_FLAG_C();										\
	SET_FLAG_Z(EQUALS_ZERO((A + (value)+FlagC) & 0xFF));			\
	SET_FLAG_N(0);													\
	SET_FLAG_H(IS_CARRY4((A & 0xF) + ((value)& 0xF) + FlagC));		\
	SET_FLAG_C(IS_CARRY8(A + (value)+FlagC));						\
	A += (value)+FlagC; }

//A - (n+С)
//Флаги:
//Z - установка, если результат 0
//N - установка
//H - установка, если без заимствования из 4 бита
//C - установка, если без заимствования
#define SBC_N(value)														\
	{int result = A - (value)-GET_FLAG_C();									\
	SET_FLAG_N(1);															\
	SET_FLAG_H(A_SMALLER_THAN_B(A & 0xF, ((value)& 0xF) + GET_FLAG_C()));	\
	SET_FLAG_C(IS_NEGATIVE(result));										\
	A = result;																\
	SET_FLAG_Z(EQUALS_ZERO(A)); }

//A - n
//Флаги:
//Z - установка, если результат 0
//N - установка
//H - установка, если без заимствования из 4 бита
//C - установка, если без заимствования
#define SUB_N(value)											\
	{SET_FLAG_Z(ARE_EQUAL(A, value));							\
	SET_FLAG_N(1);												\
	SET_FLAG_H(A_SMALLER_THAN_B(A & 0xF, (value)& 0xF));		\
	SET_FLAG_C(A_SMALLER_THAN_B(A, value)); }					\
	A -= (value);

//Логическое И
//Z - установка, если рузультат 0
//N - сброс
//H - установка
//C - сброс
#define AND_N(value)			\
	{A &= (value);				\
	SET_FLAG_Z(EQUALS_ZERO(A));	\
	SET_FLAG_N(0);				\
	SET_FLAG_H(1);				\
	SET_FLAG_C(0); }

//Логическое ИЛИ
//Z - установка, если результат 0
//N - сброс
//H - сброс
//C - сброс
#define OR_N(value)				\
	{A |= (value);				\
	SET_FLAG_Z(EQUALS_ZERO(A));	\
	SET_FLAG_N(0);				\
	SET_FLAG_H(0);				\
	SET_FLAG_C(0); }

//Логическое исключающее ИЛИ
//Z - установка, если результат 0
//N - сброс
//H - сброс
//C - сброс
#define XOR_N(value)			\
	{A ^= (value);				\
	SET_FLAG_Z(EQUALS_ZERO(A));	\
	SET_FLAG_N(0);				\
	SET_FLAG_H(0);				\
	SET_FLAG_C(0); }

//Сравнение A с n 8 бит
//Z - установка, если результат 0. (если A = n.)
//N - утановка
//H - установка, если не было заимствования из 4 бита
//C - установка, если не было заимствования. (если A < n.)
#define CP_N(value)											\
	{SET_FLAG_Z(ARE_EQUAL(A, value));						\
	SET_FLAG_N(1);											\
	SET_FLAG_H(A_SMALLER_THAN_B(A & 0xF, (value)& 0xF));	\
	SET_FLAG_C(A_SMALLER_THAN_B(A, value)); }

//Инкрементирование 8 бит
//Z - установка, если результат 0
//N - сброс
//H - установка, если произошел перенос из 3 бита
//C - не изменяется
#define INC_N(value)								\
	{SET_FLAG_Z(EQUALS_ZERO(((value)+1) & 0xFF));	\
	SET_FLAG_N(0);									\
	SET_FLAG_H(IS_CARRY4(((value)& 0xF) + 1));		\
	value++; }

//Декрементирование 8 бит
//Z - установка, если результат 0
//N - установка
//H - установка, если без заимствования из 4 бита
//C - не изменяется
#define DEC_N(value)								\
	{SET_FLAG_Z(EQUALS_ZERO((value)-1));			\
	SET_FLAG_N(1);									\
	SET_FLAG_H(IS_NEGATIVE(((value)& 0xF) - 1));	\
	(value)--; }

//Прибавляет n к HL.
//Z - не изменяется.
//N - сброс.
//H - установка, если произшел перенос из 11 бита
//C - установка, если произшел перенос из 15 бита.
#define ADD_HL(value)													\
	{SET_FLAG_N(0);														\
	SET_FLAG_H(IS_CARRY12((HL.word & 0xFFF) + ((value)& 0xFFF)));		\
	SET_FLAG_C(IS_CARRY16(HL.word + (value)));							\
	HL.word += (value);													\
	SYNC_WITH_CPU(4); }

//Инкремент регистра nn.
//nn = BC, DE, HL, SP
//Флаги не затрагиваются
#define INC_NN(value)	\
	{(value)++;			\
	SYNC_WITH_CPU(4); }

//Декремент регистра nn.
//nn = BC,DE,HL,SP
//Флаги не затрагиваются
#define DEC_NN(value)	\
	{(value)--;			\
	SYNC_WITH_CPU(4); }


#define JP_CC_NN(condition)				\
	{WORD addr = MemoryReadWord(PC);	\
	PC += 2;							\
	if ((condition))				\
	{								\
		DELAYED_JUMP(addr);			\
	}}

#define JR_CC_NN(condition)				\
	{signed char addr = MemoryRead(PC);	\
	PC++;								\
	if ((condition))					\
	{									\
		DELAYED_JUMP(PC + addr);		\
	}}

#define CALL_CC_NN(condition)			\
	{WORD addr = MemoryReadWord(PC);	\
	PC += 2;							\
	if ((condition))				\
	{								\
		StackPushWord(PC);			\
		PC = addr;					\
	}}

#define RST_N(addr)		\
	{StackPushWord(PC);	\
	PC = addr; }

#define RET_CC(condition)					\
	{SYNC_WITH_CPU(4);						\
	if ((condition))						\
	{										\
		DELAYED_JUMP(StackPopWord());		\
	}}

#define SWAP_N(value)											\
	{(value) = (((value)& 0xF) << 4) | (((value)& 0xF0) >> 4);  \
	SET_FLAG_Z(EQUALS_ZERO(value));								\
	SET_FLAG_N(0);												\
	SET_FLAG_H(0);												\
	SET_FLAG_C(0); }

#define RLC_N(value)				\
	{BYTE MSB = (value) >> 7;		\
	(value) = ((value) << 1) | MSB;	\
	SET_FLAG_Z(EQUALS_ZERO(value));	\
	SET_FLAG_H(0);					\
	SET_FLAG_N(0);					\
	SET_FLAG_C(MSB); }

#define RL_N(value)							\
	{BYTE MSB = (value) >> 7;				\
	(value) = ((value) << 1) | GET_FLAG_C();\
	SET_FLAG_Z(EQUALS_ZERO(value));			\
	SET_FLAG_N(0);							\
	SET_FLAG_H(0);							\
	SET_FLAG_C(MSB); }

#define RRC_N(value)						\
	{BYTE LSB = (value)& 0x1;				\
	(value) = ((value) >> 1) | (LSB << 7);	\
	SET_FLAG_Z(EQUALS_ZERO(value));			\
	SET_FLAG_H(0);							\
	SET_FLAG_N(0);							\
	SET_FLAG_C(LSB); }

#define RR_N(value)									\
	{BYTE LSB = (value)& 0x1;						\
	(value) = ((value) >> 1) | (GET_FLAG_C() << 7);	\
	SET_FLAG_Z(EQUALS_ZERO(value));					\
	SET_FLAG_H(0);									\
	SET_FLAG_N(0);									\
	SET_FLAG_C(LSB); }

#define SLA_N(value)				\
	{BYTE MSB = (value) >> 7;		\
	(value) <<= 1;					\
	SET_FLAG_Z(EQUALS_ZERO(value));	\
	SET_FLAG_H(0);					\
	SET_FLAG_N(0);					\
	SET_FLAG_C(MSB); }

#define SRA_N(value)							\
	{BYTE LSB = (value)& 0x1;					\
	(value) = ((value) >> 1) | ((value)& 0x80); \
	SET_FLAG_Z(EQUALS_ZERO(value));				\
	SET_FLAG_H(0);								\
	SET_FLAG_N(0);								\
	SET_FLAG_C(LSB); }

#define SRL_N(value)				\
	{BYTE LSB = (value)& 0x1;		\
	(value) >>= 1;					\
	SET_FLAG_Z(EQUALS_ZERO(value));	\
	SET_FLAG_H(0);					\
	SET_FLAG_N(0);					\
	SET_FLAG_C(LSB); }

#define BITX_N(BitIndex, value)						\
	{SET_FLAG_Z(((~(value)) >> (BitIndex)) & 0x1);	\
	SET_FLAG_N(0);									\
	SET_FLAG_H(1); }

GameBoy::CPU::CPU(
	GameBoy::Memory &MMU,
	GameBoy::GPU &GPU,
	GameBoy::DividerTimer &DIV,
	GameBoy::Joypad &joypad,
	GameBoy::Interrupts &INT) :
	MMU(MMU),
	GPU(GPU),
	DIV(DIV),
	Joypad(joypad),
	INT(INT)
{
	Reset();
}

void GameBoy::CPU::Reset()
{
	A = 0;
	PC = 0;
	F = 0;
	BC.word = 0;
	DE.word = 0;
	HL.word = 0;
	SP = 0xFFFE;
	IME = 0;
	Halted = false;
	HaltBug = false;
	DIDelay = 0;
	EIDelay = 0;
}

void GameBoy::CPU::EmulateBIOS()
{
	Reset();
	A = 0x01;
	F = 0xB0;
	BC.word = 0x0013;
	DE.word = 0x00D8;
	HL.word = 0x014D;
	SP = 0xFFFE;

	PC = 0x100;
}

void GameBoy::CPU::Step()
{
	WORD immValueW = 0;
	BYTE immValueB = 0;

	BYTE opcode = MemoryRead(PC);
	PC++;

	//Ошибка остановки при выполнении HALT при IME = 0
	//CPU продолжает работать, но инструкция после HALT
	//Выполняется дважды.
	if (!Halted && HaltBug)
	{
		HaltBug = false;
		PC--;
	}

	//Задержка до выполнения следующей инструкции
	if (DIDelay > 0)
	{
		if (DIDelay == 1)
		{
			IME = 0;
		}

		DIDelay--;
	}
	if (EIDelay > 0)
	{
		if (EIDelay == 1)
		{
			IME = 1;
		}

		EIDelay--;
	}

	switch (opcode)
	{

#pragma region LD r, n
//в регистр r значение n
	case 0x06:
		BC.bytes.H = MemoryRead(PC);
		PC++;
		break;
	case 0x0E:
		BC.bytes.L = MemoryRead(PC);
		PC++;
		break;
	case 0x16:
		DE.bytes.H = MemoryRead(PC);
		PC++;
		break;
	case 0x1E:
		DE.bytes.L = MemoryRead(PC);
		PC++;
		break;
	case 0x26:
		HL.bytes.H = MemoryRead(PC);
		PC++;
		break;
	case 0x2E:
		HL.bytes.L = MemoryRead(PC);
		PC++;
		break;
#pragma endregion
#pragma region LD r, r
//из регистра в регистр
	case 0x7F:
		A = A;
		break;
	case 0x78:
		A = BC.bytes.H;
		break;
	case 0x79:
		A = BC.bytes.L;
		break;
	case 0x7A:
		A = DE.bytes.H;
		break;
	case 0x7B:
		A = DE.bytes.L;
		break;
	case 0x7C:
		A = HL.bytes.H;
		break;
	case 0x7D:
		A = HL.bytes.L;
		break;
	case 0x7E:
		A = MemoryRead(HL.word);
		break;
	case 0x40:
		BC.bytes.H = BC.bytes.H;
		break;
	case 0x41:
		BC.bytes.H = BC.bytes.L;
		break;
	case 0x42:
		BC.bytes.H = DE.bytes.H;
		break;
	case 0x43:
		BC.bytes.H = DE.bytes.L;
		break;
	case 0x44:
		BC.bytes.H = HL.bytes.H;
		break;
	case 0x45:
		BC.bytes.H = HL.bytes.L;
		break;
	case 0x46:
		BC.bytes.H = MemoryRead(HL.word);
		break;
	case 0x48:
		BC.bytes.L = BC.bytes.H;
		break;
	case 0x49:
		BC.bytes.L = BC.bytes.L;
		break;
	case 0x4A:
		BC.bytes.L = DE.bytes.H;
		break;
	case 0x4B:
		BC.bytes.L = DE.bytes.L;
		break;
	case 0x4C:
		BC.bytes.L = HL.bytes.H;
		break;
	case 0x4D:
		BC.bytes.L = HL.bytes.L;
		break;
	case 0x4E:
		BC.bytes.L = MemoryRead(HL.word);
		break;
	case 0x50:
		DE.bytes.H = BC.bytes.H;
		break;
	case 0x51:
		DE.bytes.H = BC.bytes.L;
		break;
	case 0x52:
		DE.bytes.H = DE.bytes.H;
		break;
	case 0x53:
		DE.bytes.H = DE.bytes.L;
		break;
	case 0x54:
		DE.bytes.H = HL.bytes.H;
		break;
	case 0x55:
		DE.bytes.H = HL.bytes.L;
		break;
	case 0x56:
		DE.bytes.H = MemoryRead(HL.word);
		break;
	case 0x58:
		DE.bytes.L = BC.bytes.H;
		break;
	case 0x59:
		DE.bytes.L = BC.bytes.L;
		break;
	case 0x5A:
		DE.bytes.L = DE.bytes.H;
		break;
	case 0x5B:
		DE.bytes.L = DE.bytes.L;
		break;
	case 0x5C:
		DE.bytes.L = HL.bytes.H;
		break;
	case 0x5D:
		DE.bytes.L = HL.bytes.L;
		break;
	case 0x5E:
		DE.bytes.L = MemoryRead(HL.word);
		break;
	case 0x60:
		HL.bytes.H = BC.bytes.H;
		break;
	case 0x61:
		HL.bytes.H = BC.bytes.L;
		break;
	case 0x62:
		HL.bytes.H = DE.bytes.H;
		break;
	case 0x63:
		HL.bytes.H = DE.bytes.L;
		break;
	case 0x64:
		HL.bytes.H = HL.bytes.H;
		break;
	case 0x65:
		HL.bytes.H = HL.bytes.L;
		break;
	case 0x66:
		HL.bytes.H = MemoryRead(HL.word);
		break;
	case 0x68:
		HL.bytes.L = BC.bytes.H;
		break;
	case 0x69:
		HL.bytes.L = BC.bytes.L;
		break;
	case 0x6A:
		HL.bytes.L = DE.bytes.H;
		break;
	case 0x6B:
		HL.bytes.L = DE.bytes.L;
		break;
	case 0x6C:
		HL.bytes.L = HL.bytes.H;
		break;
	case 0x6D:
		HL.bytes.L = HL.bytes.L;
		break;
	case 0x6E:
		HL.bytes.L = MemoryRead(HL.word);
		break;
	case 0x70:
		MemoryWrite(HL.word, BC.bytes.H);
		break;
	case 0x71:
		MemoryWrite(HL.word, BC.bytes.L);
		break;
	case 0x72:
		MemoryWrite(HL.word, DE.bytes.H);
		break;
	case 0x73:
		MemoryWrite(HL.word, DE.bytes.L);
		break;
	case 0x74:
		MemoryWrite(HL.word, HL.bytes.H);
		break;
	case 0x75:
		MemoryWrite(HL.word, HL.bytes.L);
		break;
	case 0x36:
		immValueB = MemoryRead(PC);
		PC++;

		MemoryWrite(HL.word, immValueB);
		break;
#pragma endregion
#pragma region LD A, n
//значение n в регистр A
	case 0x0A:
		A = MemoryRead(BC.word);
		break;
	case 0x1A:
		A = MemoryRead(DE.word);
		break;
	case 0xFA:
		immValueW = MemoryReadWord(PC);
		PC += 2;

		A = MemoryRead(immValueW);
		break;
	case 0x3E:
		A = MemoryRead(PC);
		PC++;
		break;
#pragma endregion
#pragma region LD n, A
//из A в регистры или 2 байта непосредственно
	case 0x47:
		BC.bytes.H = A;
		break;
	case 0x4F:
		BC.bytes.L = A;
		break;
	case 0x57:
		DE.bytes.H = A;
		break;
	case 0x5F:
		DE.bytes.L = A;
		break;
	case 0x67:
		HL.bytes.H = A;
		break;
	case 0x6F:
		HL.bytes.L = A;
		break;
	case 0x02:
		MemoryWrite(BC.word, A);
		break;
	case 0x12:
		MemoryWrite(DE.word, A);
		break;
	case 0x77:
		MemoryWrite(HL.word, A);
		break;
	case 0xEA:
		immValueW = MemoryReadWord(PC);
		PC += 2;

		MemoryWrite(immValueW, A);
		break;
#pragma endregion
#pragma region LD A, (C)
//поместить значение из $FF00 + C в регистр A
	case 0xF2:
		A = MemoryRead(0xFF00 + BC.bytes.L);
		break;
#pragma endregion
#pragma region LD (C), A
//поместить значение из A в $FF00 + C
	case 0xE2:
		MemoryWrite(0xFF00 + BC.bytes.L, A);
		break;
#pragma endregion
#pragma region LDD A, (HL)
//Поместить значение из HL в A. После декрементировать HL
	case 0x3A:
		A = MemoryRead(HL.word);
		HL.word--;
		break;
#pragma endregion
#pragma region LDD (HL), A
//Поместить значение из A в HL. Декрементировать HL
	case 0x32:
		MemoryWrite(HL.word, A);
		HL.word--;
		break;
#pragma endregion
#pragma region LDI A, (HL)
//Поместить значение из HL в A. Инкрементировать HL.
	case 0x2A:
		A = MemoryRead(HL.word);
		HL.word++;
		break;
#pragma endregion
#pragma region LDI (HL), A
//Поместить A в HL. Инкрементировать HL
	case 0x22:
		MemoryWrite(HL.word, A);
		HL.word++;
		break;
#pragma endregion
#pragma region LDH (n), A
//Поместить из A в адрес $FF00+n, где n-однобайтовое непосредственное значение
	case 0xE0:
		immValueB = MemoryRead(PC);
		PC++;
		MemoryWrite(0xFF00 + immValueB, A);
		break;
#pragma endregion
#pragma region LDH A, (n)
//Поместить из адреса $FF00+n в регистр A, где n-однобайтовое непосредственное значение
	case 0xF0:
		immValueB = MemoryRead(PC);
		PC++;

		A = MemoryRead(0xFF00 + immValueB);
		break;
#pragma endregion
#pragma region LD n, nn
//Из nn в n, где n = BC, DE, HL, SP; nn = 16 bit непосредственное значение
	case 0x01:
		BC.word = MemoryReadWord(PC);
		PC += 2;
		break;
	case 0x11:
		DE.word = MemoryReadWord(PC);
		PC += 2;
		break;
	case 0x21:
		HL.word = MemoryReadWord(PC);
		PC += 2;
		break;
	case 0x31:
		SP = MemoryReadWord(PC);
		PC += 2;
		break;
#pragma endregion
#pragma region LD SP, HL
//Из HL в стэк(Stack Pointer(SP))
	case 0xF9:
		SP = HL.word;
		SYNC_WITH_CPU(4);
		break;
#pragma endregion
#pragma region LDHL SP, n
//Помещает из адреса SP+n в HL
//n-однобайтовое знаковое значение
//Затронуты флаги
//Z-сброс
//N-сброс
//H-установка или сброс в соответсвии с операцией
//C-установка или сброс в соответсвии с операцией
	case 0xF8:
		immValueB = MemoryRead(PC);
		PC++;
		immValueW = SP + (signed char)immValueB;
		SET_FLAG_Z(0);
		SET_FLAG_N(0);
		SET_FLAG_C((immValueW & 0xFF) < (SP & 0xFF));
		SET_FLAG_H((immValueW & 0xF) < (SP & 0xF));
		HL.word = immValueW;
		SYNC_WITH_CPU(4);
		break;
#pragma endregion
#pragma region LD (nn), SP
//Помещает Stack Pointer в непосредственный адрес
	case 0x08:
		immValueW = MemoryReadWord(PC);
		PC += 2;
		MemoryWriteWord(immValueW, SP);
		break;
#pragma endregion
#pragma region PUSH nn
//Помещает пары регистров AF, BC, DE, HL в stack. Декрементирование SP дважды
	case 0xF5:
		StackPushByte(A);
		StackPushByte(F);
		SYNC_WITH_CPU(4);
		break;
	case 0xC5:
		StackPushWord(BC.word);
		break;
	case 0xD5:
		StackPushWord(DE.word);
		break;
	case 0xE5:
		StackPushWord(HL.word);
		break;
#pragma endregion
#pragma region POP nn
//Вытаскивает 2 байта из стека в парные регистры AF, BC, DE, HL. Инкрементирует SP дважды
	case 0xF1:
		F = StackPopByte();
		A = StackPopByte();
		break;
	case 0xC1:
		BC.word = StackPopWord();
		break;
	case 0xD1:
		DE.word = StackPopWord();
		break;
	case 0xE1:
		HL.word = StackPopWord();
		break;
#pragma endregion
#pragma region ADD A, n
//Прибавляет n к A
//n = A, B, C, D, E, H, L, (HL), #
	case 0x87:
		ADD_N(A);
		break;
	case 0x80:
		ADD_N(BC.bytes.H);
		break;
	case 0x81:
		ADD_N(BC.bytes.L);
		break;
	case 0x82:
		ADD_N(DE.bytes.H);
		break;
	case 0x83:
		ADD_N(DE.bytes.L);
		break;
	case 0x84:
		ADD_N(HL.bytes.H);
		break;
	case 0x85:
		ADD_N(HL.bytes.L);
		break;
	case 0x86:
		immValueB = MemoryRead(HL.word);
		ADD_N(immValueB);
		break;
	case 0xC6:
		immValueB = MemoryRead(PC);
		PC++;
		ADD_N(immValueB);
		break;
#pragma endregion
#pragma region ADC A, n
//n+флаг переноса в A, где n = A, B, C, D, E, H, L, (HL), #
	case 0x8F:
		ADC_N(A);
		break;
	case 0x88:
		ADC_N(BC.bytes.H);
		break;
	case 0x89:
		ADC_N(BC.bytes.L);
		break;
	case 0x8A:
		ADC_N(DE.bytes.H);
		break;
	case 0x8B:
		ADC_N(DE.bytes.L);
		break;
	case 0x8C:
		ADC_N(HL.bytes.H);
		break;
	case 0x8D:
		ADC_N(HL.bytes.L);
		break;
	case 0x8E:
		immValueB = MemoryRead(HL.word);
		ADC_N(immValueB);
		break;
	case 0xCE:
		immValueB = MemoryRead(PC);
		PC++;
		ADC_N(immValueB);
		break;
#pragma endregion
#pragma region SUB n
//Вычитание n из A, где n = A, B, C, D, E, H, L, (HL), #
	case 0x97:
		SUB_N(A);
		break;
	case 0x90:
		SUB_N(BC.bytes.H);
		break;
	case 0x91:
		SUB_N(BC.bytes.L);
		break;
	case 0x92:
		SUB_N(DE.bytes.H);
		break;
	case 0x93:
		SUB_N(DE.bytes.L);
		break;
	case 0x94:
		SUB_N(HL.bytes.H);
		break;
	case 0x95:
		SUB_N(HL.bytes.L);
		break;
	case 0x96:
		immValueB = MemoryRead(HL.word);
		SUB_N(immValueB);
		break;
	case 0xD6:
		immValueB = MemoryRead(PC);
		PC++;
		SUB_N(immValueB);
		break;
#pragma endregion
#pragma region SBC A, n
//Вычитание n + Флаг переноса C из A, где n = A, B, C, D, E, H, L, (HL), #
	case 0x9F:
		SBC_N(A);
		break;
	case 0x98:
		SBC_N(BC.bytes.H);
		break;
	case 0x99:
		SBC_N(BC.bytes.L);
		break;
	case 0x9A:
		SBC_N(DE.bytes.H);
		break;
	case 0x9B:
		SBC_N(DE.bytes.L);
		break;
	case 0x9C:
		SBC_N(HL.bytes.H);
		break;
	case 0x9D:
		SBC_N(HL.bytes.L);
		break;
	case 0x9E:
		immValueB = MemoryRead(HL.word);
		SBC_N(immValueB);
		break;
	case 0xDE:
		immValueB = MemoryRead(PC);
		PC++;
		SBC_N(immValueB);
		break;
#pragma endregion
#pragma region AND n
//Логическое И
	case 0xA7:
		AND_N(A);
		break;
	case 0xA0:
		AND_N(BC.bytes.H);
		break;
	case 0xA1:
		AND_N(BC.bytes.L);
		break;
	case 0xA2:
		AND_N(DE.bytes.H);
		break;
	case 0xA3:
		AND_N(DE.bytes.L);
		break;
	case 0xA4:
		AND_N(HL.bytes.H);
		break;
	case 0xA5:
		AND_N(HL.bytes.L);
		break;
	case 0xA6:
		immValueB = MemoryRead(HL.word);
		AND_N(immValueB);
		break;
	case 0xE6:
		immValueB = MemoryRead(PC);
		PC++;
		AND_N(immValueB);
		break;
#pragma endregion
#pragma region OR n
//Логическое ИЛИ
	case 0xB7:
		OR_N(A);
		break;
	case 0xB0:
		OR_N(BC.bytes.H);
		break;
	case 0xB1:
		OR_N(BC.bytes.L);
		break;
	case 0xB2:
		OR_N(DE.bytes.H);
		break;
	case 0xB3:
		OR_N(DE.bytes.L);
		break;
	case 0xB4:
		OR_N(HL.bytes.H);
		break;
	case 0xB5:
		OR_N(HL.bytes.L);
		break;
	case 0xB6:
		immValueB = MemoryRead(HL.word);
		OR_N(immValueB);
		break;
	case 0xF6:
		immValueB = MemoryRead(PC);
		PC++;
		OR_N(immValueB);
		break;
#pragma endregion
#pragma region XOR n
//Логическое исключающее ИЛИ
	case 0xAF:
		XOR_N(A);
		break;
	case 0xA8:
		XOR_N(BC.bytes.H);
		break;
	case 0xA9:
		XOR_N(BC.bytes.L);
		break;
	case 0xAA:
		XOR_N(DE.bytes.H);
		break;
	case 0xAB:
		XOR_N(DE.bytes.L);
		break;
	case 0xAC:
		XOR_N(HL.bytes.H);
		break;
	case 0xAD:
		XOR_N(HL.bytes.L);
		break;
	case 0xAE:
		immValueB = MemoryRead(HL.word);
		XOR_N(immValueB);
		break;
	case 0xEE:
		immValueB = MemoryRead(PC);
		PC++;
		XOR_N(immValueB);
		break;
#pragma endregion
#pragma region CP n
//Сравнивает A с n
	case 0xBF:
		CP_N(A);
		break;
	case 0xB8:
		CP_N(BC.bytes.H);
		break;
	case 0xB9:
		CP_N(BC.bytes.L);
		break;
	case 0xBA:
		CP_N(DE.bytes.H);
		break;
	case 0xBB:
		CP_N(DE.bytes.L);
		break;
	case 0xBC:
		CP_N(HL.bytes.H);
		break;
	case 0xBD:
		CP_N(HL.bytes.L);
		break;
	case 0xBE:
		immValueB = MemoryRead(HL.word);
		CP_N(immValueB);
		break;
	case 0xFE:
		immValueB = MemoryRead(PC);
		PC++;
		CP_N(immValueB);
		break;
#pragma endregion
#pragma region INC n
//Инкрементирование регистра
	case 0x3C:
		INC_N(A);
		break;
	case 0x04:
		INC_N(BC.bytes.H);
		break;
	case 0x0C:
		INC_N(BC.bytes.L);
		break;
	case 0x14:
		INC_N(DE.bytes.H);
		break;
	case 0x1C:
		INC_N(DE.bytes.L);
		break;
	case 0x24:
		INC_N(HL.bytes.H);
		break;
	case 0x2C:
		INC_N(HL.bytes.L);
		break;
	case 0x34:
		immValueB = MemoryRead(HL.word);
		INC_N(immValueB);
		MemoryWrite(HL.word, immValueB);
		break;
#pragma endregion
#pragma region DEC n
//Декрементирование регистров
	case 0x3D:
		DEC_N(A);
		break;
	case 0x05:
		DEC_N(BC.bytes.H);
		break;
	case 0x0D:
		DEC_N(BC.bytes.L);
		break;
	case 0x15:
		DEC_N(DE.bytes.H);
		break;
	case 0x1D:
		DEC_N(DE.bytes.L);
		break;
	case 0x25:
		DEC_N(HL.bytes.H);
		break;
	case 0x2D:
		DEC_N(HL.bytes.L);
		break;
	case 0x35:
		immValueB = MemoryRead(HL.word);
		DEC_N(immValueB);
		MemoryWrite(HL.word, immValueB);
		break;
#pragma endregion
#pragma region ADD HL, n
//прибавляет n к HL, где n = BC,DE,HL,SP
	case 0x09:
		ADD_HL(BC.word);
		break;
	case 0x19:
		ADD_HL(DE.word);
		break;
	case 0x29:
		ADD_HL(HL.word);
		break;
	case 0x39:
		ADD_HL(SP);
		break;
#pragma endregion
#pragma region ADD SP, n
//Прибавляет n к Stack Pointer
//n = один байт знакового непосредственного значения(#)
//Z - сброс
//N - сброс
//H - установка или сброс в соответствии с операцией
//C - установка или сброс в соответствии с операцией
	case 0xE8:
		immValueB = MemoryRead(PC);
		PC++;
		immValueW = SP + (signed char)immValueB;
		SET_FLAG_Z(0);
		SET_FLAG_N(0);
		SET_FLAG_H((immValueW & 0xF) < (SP & 0xF));
		SET_FLAG_C((immValueW & 0xFF) < (SP & 0xFF));
		SP = immValueW;
		SYNC_WITH_CPU(8);
		break;
#pragma endregion
#pragma region INC nn
//Инкремент регистра nn
//nn = BC,DE,HL,SP
	case 0x03:
		INC_NN(BC.word);
		break;
	case 0x13:
		INC_NN(DE.word);
		break;
	case 0x23:
		INC_NN(HL.word);
		break;
	case 0x33:
		INC_NN(SP);
		break;
#pragma endregion
#pragma region DEC nn
//Декремент регистра nn.
//nn = BC,DE,HL,SP
	case 0x0B:
		DEC_NN(BC.word);
		break;
	case 0x1B:
		DEC_NN(DE.word);
		break;
	case 0x2B:
		DEC_NN(HL.word);
		break;
	case 0x3B:
		DEC_NN(SP);
		break;
#pragma endregion
#pragma region DAA
//Эта инструкция корректирует регистр A, чтобы получить правильное представление BCD
//Z - Установка, если A = 0
//N - не затрагивается
//H - сброс
//C - установка или сброс в соответствии с операцией
	case 0x27:
	{
			int A = this->A;
			if (GET_FLAG_N() == 0)
			{
				if (GET_FLAG_H() != 0 || (A & 0xF) > 9)
				{
					A += 0x06;
				}
				if (GET_FLAG_C() != 0 || A > 0x9F)
				{
					A += 0x60;
				}
			}
			else
			{
				if (GET_FLAG_H() != 0)
				{
					A = (A - 6) & 0xFF;
				}
				if (GET_FLAG_C() != 0)
				{
					A -= 0x60;
				}
			}
			SET_FLAG_H(0);
			if ((A & 0x100) == 0x100)
			{
				SET_FLAG_C(1);
			}
			A &= 0xFF;
			SET_FLAG_Z(A == 0);
			this->A = A;
	}
	break;
#pragma endregion
#pragma region CPL
//Побитовая инверсия регистра А
//Z - не затрагивается
//N - установка
//H - установка
//C - не затрагивается
	case 0x2F:
		A = ~A;
		SET_FLAG_N(1);
		SET_FLAG_H(1);
		break;
#pragma endregion
#pragma region CCF
//Меняет флаг переноса(Carry flag)
//set->reset/reset->set
//Z - не затрагивается
//N - сброс
//H - сброс
//C - инвертирование
	case 0x3F:
		SET_FLAG_N(0);
		SET_FLAG_H(0);
		SET_FLAG_C(1 - GET_FLAG_C());
		break;
#pragma endregion
#pragma region SCF
//Устанвка С
	case 0x37:
		SET_FLAG_N(0);
		SET_FLAG_H(0);
		SET_FLAG_C(1);
		break;
#pragma endregion
#pragma region NOP
//Нет операций
	case 0x00:
		break;
#pragma endregion
#pragma region HALT
//Выключение питания CPU до выполнения прерывания. Используется, когда моожно снизить потребление энергии
	case 0x76:
		if (!Halted)
		{
			if (IME == 0 && (INT.GetIE() & INT.GetIF() & 0x1F))
			{
				HaltBug = true;
			}
			else
			{
				Halted = true;
				PC--;
			}
		}
		else
		{
			PC--;
		}

		break;
#pragma endregion
#pragma region STOP
//Остановка CPU и LCD дисплея
	case 0x10:
		break;
#pragma endregion
#pragma region DI
//Отключение прерываний
	case 0xF3:
		//Задержка до выполнения следующей команды
		DIDelay = 2;
		break;
#pragma endregion
#pragma region EI
//Включение прерываний
	case 0xFB:
		//задержка до выполнения следующей команды
		EIDelay = 2;
		break;
#pragma endregion
//------------
#pragma region RLCA
	case 0x07:
		RLC_N(A);

		SET_FLAG_Z(0);
		break;
#pragma endregion
#pragma region RLA
	case 0x17:
		RL_N(A);

		SET_FLAG_Z(0);
		break;
#pragma endregion
#pragma region RRCA
	case 0x0F:
		RRC_N(A);

		SET_FLAG_Z(0);
		break;
#pragma endregion
#pragma region RRA
	case 0x1F:
		RR_N(A);

		SET_FLAG_Z(0);
		break;
#pragma endregion
#pragma region JP nn
	case 0xC3:
		immValueW = MemoryReadWord(PC);
		PC += 2;

		DELAYED_JUMP(immValueW);
		break;
#pragma endregion
#pragma region JP cc, nn
	case 0xC2:
		JP_CC_NN(GET_FLAG_Z() == 0);
		break;
	case 0xCA:
		JP_CC_NN(GET_FLAG_Z() == 1);
		break;
	case 0xD2:
		JP_CC_NN(GET_FLAG_C() == 0);
		break;
	case 0xDA:
		JP_CC_NN(GET_FLAG_C() == 1);
		break;
#pragma endregion
#pragma region JP (HL)
	case 0xE9:
		PC = HL.word;
		break;
#pragma endregion
#pragma region JR n
	case 0x18:
		immValueB = MemoryRead(PC);
		PC++;

		DELAYED_JUMP(PC + (signed char)immValueB);
		break;
#pragma endregion
#pragma region JR cc, n
	case 0x20:
		JR_CC_NN(GET_FLAG_Z() == 0);
		break;
	case 0x28:
		JR_CC_NN(GET_FLAG_Z() == 1);
		break;
	case 0x30:
		JR_CC_NN(GET_FLAG_C() == 0);
		break;
	case 0x38:
		JR_CC_NN(GET_FLAG_C() == 1);
		break;
#pragma endregion
#pragma region CALL nn
	case 0xCD:
		immValueW = MemoryReadWord(PC);
		PC += 2;

		StackPushWord(PC);

		PC = immValueW;
		break;
#pragma endregion
#pragma region CALL cc, nn
	case 0xC4:
		CALL_CC_NN(GET_FLAG_Z() == 0);
		break;
	case 0xCC:
		CALL_CC_NN(GET_FLAG_Z() == 1);
		break;
	case 0xD4:
		CALL_CC_NN(GET_FLAG_C() == 0);
		break;
	case 0xDC:
		CALL_CC_NN(GET_FLAG_C() == 1);
		break;
#pragma endregion
#pragma region RST n
	case 0xC7:
		RST_N(0x00);
		break;
	case 0xCF:
		RST_N(0x08);
		break;
	case 0xD7:
		RST_N(0x10);
		break;
	case 0xDF:
		RST_N(0x18);
		break;
	case 0xE7:
		RST_N(0x20);
		break;
	case 0xEF:
		RST_N(0x28);
		break;
	case 0xF7:
		RST_N(0x30);
		break;
	case 0xFF:
		RST_N(0x38);
		break;
#pragma endregion
#pragma region RET
	case 0xC9:
		DELAYED_JUMP(StackPopWord());
		break;
#pragma endregion
#pragma region RET cc
	case 0xC0:
		RET_CC(GET_FLAG_Z() == 0);
		break;
	case 0xC8:
		RET_CC(GET_FLAG_Z() == 1);
		break;
	case 0xD0:
		RET_CC(GET_FLAG_C() == 0);
		break;
	case 0xD8:
		RET_CC(GET_FLAG_C() == 1);
		break;
#pragma endregion
#pragma region RETI
	case 0xD9:
		DELAYED_JUMP(StackPopWord());
		IME = 1;
		break;
#pragma endregion
#pragma region CB prefix
	case 0xCB:
		opcode = MemoryRead(PC);
		PC++;

		switch (opcode)
		{
#pragma region SWAP n
		case 0x37:
			SWAP_N(A);
			break;
		case 0x30:
			SWAP_N(BC.bytes.H);
			break;
		case 0x31:
			SWAP_N(BC.bytes.L);
			break;
		case 0x32:
			SWAP_N(DE.bytes.H);
			break;
		case 0x33:
			SWAP_N(DE.bytes.L);
			break;
		case 0x34:
			SWAP_N(HL.bytes.H);
			break;
		case 0x35:
			SWAP_N(HL.bytes.L);
			break;
		case 0x36:
			immValueB = MemoryRead(HL.word);

			SWAP_N(immValueB);

			MemoryWrite(HL.word, immValueB);
			break;
#pragma endregion
#pragma region RLC n
		case 0x07:
			RLC_N(A);
			break;
		case 0x00:
			RLC_N(BC.bytes.H);
			break;
		case 0x01:
			RLC_N(BC.bytes.L);
			break;
		case 0x02:
			RLC_N(DE.bytes.H);
			break;
		case 0x03:
			RLC_N(DE.bytes.L);
			break;
		case 0x04:
			RLC_N(HL.bytes.H);
			break;
		case 0x05:
			RLC_N(HL.bytes.L);
			break;
		case 0x06:
			immValueB = MemoryRead(HL.word);

			RLC_N(immValueB);

			MemoryWrite(HL.word, immValueB);
			break;
#pragma endregion
#pragma region RL n
		case 0x17:
			RL_N(A);
			break;
		case 0x10:
			RL_N(BC.bytes.H);
			break;
		case 0x11:
			RL_N(BC.bytes.L);
			break;
		case 0x12:
			RL_N(DE.bytes.H);
			break;
		case 0x13:
			RL_N(DE.bytes.L);
			break;
		case 0x14:
			RL_N(HL.bytes.H);
			break;
		case 0x15:
			RL_N(HL.bytes.L);
			break;
		case 0x16:
			immValueB = MemoryRead(HL.word);

			RL_N(immValueB);

			MemoryWrite(HL.word, immValueB);
			break;
#pragma endregion
#pragma region RRC n
		case 0x0F:
			RRC_N(A);
			break;
		case 0x08:
			RRC_N(BC.bytes.H);
			break;
		case 0x09:
			RRC_N(BC.bytes.L);
			break;
		case 0x0A:
			RRC_N(DE.bytes.H);
			break;
		case 0x0B:
			RRC_N(DE.bytes.L);
			break;
		case 0x0C:
			RRC_N(HL.bytes.H);
			break;
		case 0x0D:
			RRC_N(HL.bytes.L);
			break;
		case 0x0E:
			immValueB = MemoryRead(HL.word);

			RRC_N(immValueB);

			MemoryWrite(HL.word, immValueB);
			break;
#pragma endregion
#pragma region RR n
		case 0x1F:
			RR_N(A);
			break;
		case 0x18:
			RR_N(BC.bytes.H);
			break;
		case 0x19:
			RR_N(BC.bytes.L);
			break;
		case 0x1A:
			RR_N(DE.bytes.H);
			break;
		case 0x1B:
			RR_N(DE.bytes.L);
			break;
		case 0x1C:
			RR_N(HL.bytes.H);
			break;
		case 0x1D:
			RR_N(HL.bytes.L);
			break;
		case 0x1E:
			immValueB = MemoryRead(HL.word);

			RR_N(immValueB);

			MemoryWrite(HL.word, immValueB);
			break;
#pragma endregion
#pragma region SLA n
		case 0x27:
			SLA_N(A);
			break;
		case 0x20:
			SLA_N(BC.bytes.H);
			break;
		case 0x21:
			SLA_N(BC.bytes.L);
			break;
		case 0x22:
			SLA_N(DE.bytes.H);
			break;
		case 0x23:
			SLA_N(DE.bytes.L);
			break;
		case 0x24:
			SLA_N(HL.bytes.H);
			break;
		case 0x25:
			SLA_N(HL.bytes.L);
			break;
		case 0x26:
			immValueB = MemoryRead(HL.word);

			SLA_N(immValueB);

			MemoryWrite(HL.word, immValueB);
			break;
#pragma endregion
#pragma region SRA n
		case 0x2F:
			SRA_N(A);
			break;
		case 0x28:
			SRA_N(BC.bytes.H);
			break;
		case 0x29:
			SRA_N(BC.bytes.L);
			break;
		case 0x2A:
			SRA_N(DE.bytes.H);
			break;
		case 0x2B:
			SRA_N(DE.bytes.L);
			break;
		case 0x2C:
			SRA_N(HL.bytes.H);
			break;
		case 0x2D:
			SRA_N(HL.bytes.L);
			break;
		case 0x2E:
			immValueB = MemoryRead(HL.word);

			SRA_N(immValueB);

			MemoryWrite(HL.word, immValueB);
			break;
#pragma endregion
#pragma region SRL n
		case 0x3F:
			SRL_N(A);
			break;
		case 0x38:
			SRL_N(BC.bytes.H);
			break;
		case 0x39:
			SRL_N(BC.bytes.L);
			break;
		case 0x3A:
			SRL_N(DE.bytes.H);
			break;
		case 0x3B:
			SRL_N(DE.bytes.L);
			break;
		case 0x3C:
			SRL_N(HL.bytes.H);
			break;
		case 0x3D:
			SRL_N(HL.bytes.L);
			break;
		case 0x3E:
			immValueB = MemoryRead(HL.word);

			SRL_N(immValueB);

			MemoryWrite(HL.word, immValueB);
			break;
#pragma endregion
#pragma region BIT 0, n
		case 0x47:
			BITX_N(0, A);
			break;
		case 0x40:
			BITX_N(0, BC.bytes.H);
			break;
		case 0x41:
			BITX_N(0, BC.bytes.L);
			break;
		case 0x42:
			BITX_N(0, DE.bytes.H);
			break;
		case 0x43:
			BITX_N(0, DE.bytes.L);
			break;
		case 0x44:
			BITX_N(0, HL.bytes.H);
			break;
		case 0x45:
			BITX_N(0, HL.bytes.L);
			break;
		case 0x46:
			immValueB = MemoryRead(HL.word);

			BITX_N(0, immValueB);
			break;
#pragma endregion
#pragma region BIT 1, n
		case 0x4F:
			BITX_N(1, A);
			break;
		case 0x48:
			BITX_N(1, BC.bytes.H);
			break;
		case 0x49:
			BITX_N(1, BC.bytes.L);
			break;
		case 0x4A:
			BITX_N(1, DE.bytes.H);
			break;
		case 0x4B:
			BITX_N(1, DE.bytes.L);
			break;
		case 0x4C:
			BITX_N(1, HL.bytes.H);
			break;
		case 0x4D:
			BITX_N(1, HL.bytes.L);
			break;
		case 0x4E:
			immValueB = MemoryRead(HL.word);

			BITX_N(1, immValueB);
			break;
#pragma endregion
#pragma region BIT 2, n
		case 0x57:
			BITX_N(2, A);
			break;
		case 0x50:
			BITX_N(2, BC.bytes.H);
			break;
		case 0x51:
			BITX_N(2, BC.bytes.L);
			break;
		case 0x52:
			BITX_N(2, DE.bytes.H);
			break;
		case 0x53:
			BITX_N(2, DE.bytes.L);
			break;
		case 0x54:
			BITX_N(2, HL.bytes.H);
			break;
		case 0x55:
			BITX_N(2, HL.bytes.L);
			break;
		case 0x56:
			immValueB = MemoryRead(HL.word);

			BITX_N(2, immValueB);
			break;
#pragma endregion
#pragma region BIT 3, n
		case 0x5F:
			BITX_N(3, A);
			break;
		case 0x58:
			BITX_N(3, BC.bytes.H);
			break;
		case 0x59:
			BITX_N(3, BC.bytes.L);
			break;
		case 0x5A:
			BITX_N(3, DE.bytes.H);
			break;
		case 0x5B:
			BITX_N(3, DE.bytes.L);
			break;
		case 0x5C:
			BITX_N(3, HL.bytes.H);
			break;
		case 0x5D:
			BITX_N(3, HL.bytes.L);
			break;
		case 0x5E:
			immValueB = MemoryRead(HL.word);

			BITX_N(3, immValueB);
			break;
#pragma endregion
#pragma region BIT 4, n
		case 0x67:
			BITX_N(4, A);
			break;
		case 0x60:
			BITX_N(4, BC.bytes.H);
			break;
		case 0x61:
			BITX_N(4, BC.bytes.L);
			break;
		case 0x62:
			BITX_N(4, DE.bytes.H);
			break;
		case 0x63:
			BITX_N(4, DE.bytes.L);
			break;
		case 0x64:
			BITX_N(4, HL.bytes.H);
			break;
		case 0x65:
			BITX_N(4, HL.bytes.L);
			break;
		case 0x66:
			immValueB = MemoryRead(HL.word);

			BITX_N(4, immValueB);
			break;
#pragma endregion
#pragma region BIT 5, n
		case 0x6F:
			BITX_N(5, A);
			break;
		case 0x68:
			BITX_N(5, BC.bytes.H);
			break;
		case 0x69:
			BITX_N(5, BC.bytes.L);
			break;
		case 0x6A:
			BITX_N(5, DE.bytes.H);
			break;
		case 0x6B:
			BITX_N(5, DE.bytes.L);
			break;
		case 0x6C:
			BITX_N(5, HL.bytes.H);
			break;
		case 0x6D:
			BITX_N(5, HL.bytes.L);
			break;
		case 0x6E:
			immValueB = MemoryRead(HL.word);

			BITX_N(5, immValueB);
			break;
#pragma endregion
#pragma region BIT 6, n
		case 0x77:
			BITX_N(6, A);
			break;
		case 0x70:
			BITX_N(6, BC.bytes.H);
			break;
		case 0x71:
			BITX_N(6, BC.bytes.L);
			break;
		case 0x72:
			BITX_N(6, DE.bytes.H);
			break;
		case 0x73:
			BITX_N(6, DE.bytes.L);
			break;
		case 0x74:
			BITX_N(6, HL.bytes.H);
			break;
		case 0x75:
			BITX_N(6, HL.bytes.L);
			break;
		case 0x76:
			immValueB = MemoryRead(HL.word);

			BITX_N(6, immValueB);
			break;
#pragma endregion
#pragma region BIT 7, n
		case 0x7F:
			BITX_N(7, A);
			break;
		case 0x78:
			BITX_N(7, BC.bytes.H);
			break;
		case 0x79:
			BITX_N(7, BC.bytes.L);
			break;
		case 0x7A:
			BITX_N(7, DE.bytes.H);
			break;
		case 0x7B:
			BITX_N(7, DE.bytes.L);
			break;
		case 0x7C:
			BITX_N(7, HL.bytes.H);
			break;
		case 0x7D:
			BITX_N(7, HL.bytes.L);
			break;
		case 0x7E:
			immValueB = MemoryRead(HL.word);

			BITX_N(7, immValueB);
			break;
#pragma endregion
#pragma region SET 0, n
		case 0xC7:
			A |= 0x1;
			break;
		case 0xC0:
			BC.bytes.H |= 0x1;
			break;
		case 0xC1:
			BC.bytes.L |= 0x1;
			break;
		case 0xC2:
			DE.bytes.H |= 0x1;
			break;
		case 0xC3:
			DE.bytes.L |= 0x1;
			break;
		case 0xC4:
			HL.bytes.H |= 0x1;
			break;
		case 0xC5:
			HL.bytes.L |= 0x1;
			break;
		case 0xC6:
			immValueB = MemoryRead(HL.word) | 0x1;

			MemoryWrite(HL.word, immValueB);
			break;
#pragma endregion
#pragma region SET 1, n
		case 0xCF:
			A |= 0x2;
			break;
		case 0xC8:
			BC.bytes.H |= 0x2;
			break;
		case 0xC9:
			BC.bytes.L |= 0x2;
			break;
		case 0xCA:
			DE.bytes.H |= 0x2;
			break;
		case 0xCB:
			DE.bytes.L |= 0x2;
			break;
		case 0xCC:
			HL.bytes.H |= 0x2;
			break;
		case 0xCD:
			HL.bytes.L |= 0x2;
			break;
		case 0xCE:
			immValueB = MemoryRead(HL.word) | 0x2;

			MemoryWrite(HL.word, immValueB);
			break;
#pragma endregion
#pragma region SET 2, n
		case 0xD7:
			A |= 0x4;
			break;
		case 0xD0:
			BC.bytes.H |= 0x4;
			break;
		case 0xD1:
			BC.bytes.L |= 0x4;
			break;
		case 0xD2:
			DE.bytes.H |= 0x4;
			break;
		case 0xD3:
			DE.bytes.L |= 0x4;
			break;
		case 0xD4:
			HL.bytes.H |= 0x4;
			break;
		case 0xD5:
			HL.bytes.L |= 0x4;
			break;
		case 0xD6:
			immValueB = MemoryRead(HL.word) | 0x4;

			MemoryWrite(HL.word, immValueB);
			break;
#pragma endregion
#pragma region SET 3, n
		case 0xDF:
			A |= 0x8;
			break;
		case 0xD8:
			BC.bytes.H |= 0x8;
			break;
		case 0xD9:
			BC.bytes.L |= 0x8;
			break;
		case 0xDA:
			DE.bytes.H |= 0x8;
			break;
		case 0xDB:
			DE.bytes.L |= 0x8;
			break;
		case 0xDC:
			HL.bytes.H |= 0x8;
			break;
		case 0xDD:
			HL.bytes.L |= 0x8;
			break;
		case 0xDE:
			immValueB = MemoryRead(HL.word) | 0x8;

			MemoryWrite(HL.word, immValueB);
			break;
#pragma endregion
#pragma region SET 4, n
		case 0xE7:
			A |= 0x10;
			break;
		case 0xE0:
			BC.bytes.H |= 0x10;
			break;
		case 0xE1:
			BC.bytes.L |= 0x10;
			break;
		case 0xE2:
			DE.bytes.H |= 0x10;
			break;
		case 0xE3:
			DE.bytes.L |= 0x10;
			break;
		case 0xE4:
			HL.bytes.H |= 0x10;
			break;
		case 0xE5:
			HL.bytes.L |= 0x10;
			break;
		case 0xE6:
			immValueB = MemoryRead(HL.word) | 0x10;

			MemoryWrite(HL.word, immValueB);
			break;
#pragma endregion
#pragma region SET 5, n
		case 0xEF:
			A |= 0x20;
			break;
		case 0xE8:
			BC.bytes.H |= 0x20;
			break;
		case 0xE9:
			BC.bytes.L |= 0x20;
			break;
		case 0xEA:
			DE.bytes.H |= 0x20;
			break;
		case 0xEB:
			DE.bytes.L |= 0x20;
			break;
		case 0xEC:
			HL.bytes.H |= 0x20;
			break;
		case 0xED:
			HL.bytes.L |= 0x20;
			break;
		case 0xEE:
			immValueB = MemoryRead(HL.word) | 0x20;

			MemoryWrite(HL.word, immValueB);
			break;
#pragma endregion
#pragma region SET 6, n
		case 0xF7:
			A |= 0x40;
			break;
		case 0xF0:
			BC.bytes.H |= 0x40;
			break;
		case 0xF1:
			BC.bytes.L |= 0x40;
			break;
		case 0xF2:
			DE.bytes.H |= 0x40;
			break;
		case 0xF3:
			DE.bytes.L |= 0x40;
			break;
		case 0xF4:
			HL.bytes.H |= 0x40;
			break;
		case 0xF5:
			HL.bytes.L |= 0x40;
			break;
		case 0xF6:
			immValueB = MemoryRead(HL.word) | 0x40;

			MemoryWrite(HL.word, immValueB);
			break;
#pragma endregion
#pragma region SET 7, n
		case 0xFF:
			A |= 0x80;
			break;
		case 0xF8:
			BC.bytes.H |= 0x80;
			break;
		case 0xF9:
			BC.bytes.L |= 0x80;
			break;
		case 0xFA:
			DE.bytes.H |= 0x80;
			break;
		case 0xFB:
			DE.bytes.L |= 0x80;
			break;
		case 0xFC:
			HL.bytes.H |= 0x80;
			break;
		case 0xFD:
			HL.bytes.L |= 0x80;
			break;
		case 0xFE:
			immValueB = MemoryRead(HL.word) | 0x80;

			MemoryWrite(HL.word, immValueB);
			break;
#pragma endregion
#pragma region RES 0, n
		case 0x87:
			A &= 0xFE;
			break;
		case 0x80:
			BC.bytes.H &= 0xFE;
			break;
		case 0x81:
			BC.bytes.L &= 0xFE;
			break;
		case 0x82:
			DE.bytes.H &= 0xFE;
			break;
		case 0x83:
			DE.bytes.L &= 0xFE;
			break;
		case 0x84:
			HL.bytes.H &= 0xFE;
			break;
		case 0x85:
			HL.bytes.L &= 0xFE;
			break;
		case 0x86:
			immValueB = MemoryRead(HL.word) & 0xFE;

			MemoryWrite(HL.word, immValueB);
			break;
#pragma endregion
#pragma region RES 1, n
		case 0x8F:
			A &= 0xFD;
			break;
		case 0x88:
			BC.bytes.H &= 0xFD;
			break;
		case 0x89:
			BC.bytes.L &= 0xFD;
			break;
		case 0x8A:
			DE.bytes.H &= 0xFD;
			break;
		case 0x8B:
			DE.bytes.L &= 0xFD;
			break;
		case 0x8C:
			HL.bytes.H &= 0xFD;
			break;
		case 0x8D:
			HL.bytes.L &= 0xFD;
			break;
		case 0x8E:
			immValueB = MemoryRead(HL.word) & 0xFD;

			MemoryWrite(HL.word, immValueB);
			break;
#pragma endregion
#pragma region RES 2, n
		case 0x97:
			A &= 0xFB;
			break;
		case 0x90:
			BC.bytes.H &= 0xFB;
			break;
		case 0x91:
			BC.bytes.L &= 0xFB;
			break;
		case 0x92:
			DE.bytes.H &= 0xFB;
			break;
		case 0x93:
			DE.bytes.L &= 0xFB;
			break;
		case 0x94:
			HL.bytes.H &= 0xFB;
			break;
		case 0x95:
			HL.bytes.L &= 0xFB;
			break;
		case 0x96:
			immValueB = MemoryRead(HL.word) & 0xFB;

			MemoryWrite(HL.word, immValueB);
			break;
#pragma endregion
#pragma region RES 3, n
		case 0x9F:
			A &= 0xF7;
			break;
		case 0x98:
			BC.bytes.H &= 0xF7;
			break;
		case 0x99:
			BC.bytes.L &= 0xF7;
			break;
		case 0x9A:
			DE.bytes.H &= 0xF7;
			break;
		case 0x9B:
			DE.bytes.L &= 0xF7;
			break;
		case 0x9C:
			HL.bytes.H &= 0xF7;
			break;
		case 0x9D:
			HL.bytes.L &= 0xF7;
			break;
		case 0x9E:
			immValueB = MemoryRead(HL.word) & 0xF7;

			MemoryWrite(HL.word, immValueB);
			break;
#pragma endregion
#pragma region RES 4, n
		case 0xA7:
			A &= 0xEF;
			break;
		case 0xA0:
			BC.bytes.H &= 0xEF;
			break;
		case 0xA1:
			BC.bytes.L &= 0xEF;
			break;
		case 0xA2:
			DE.bytes.H &= 0xEF;
			break;
		case 0xA3:
			DE.bytes.L &= 0xEF;
			break;
		case 0xA4:
			HL.bytes.H &= 0xEF;
			break;
		case 0xA5:
			HL.bytes.L &= 0xEF;
			break;
		case 0xA6:
			immValueB = MemoryRead(HL.word) & 0xEF;

			MemoryWrite(HL.word, immValueB);
			break;
#pragma endregion
#pragma region RES 5, n
		case 0xAF:
			A &= 0xDF;
			break;
		case 0xA8:
			BC.bytes.H &= 0xDF;
			break;
		case 0xA9:
			BC.bytes.L &= 0xDF;
			break;
		case 0xAA:
			DE.bytes.H &= 0xDF;
			break;
		case 0xAB:
			DE.bytes.L &= 0xDF;
			break;
		case 0xAC:
			HL.bytes.H &= 0xDF;
			break;
		case 0xAD:
			HL.bytes.L &= 0xDF;
			break;
		case 0xAE:
			immValueB = MemoryRead(HL.word) & 0xDF;

			MemoryWrite(HL.word, immValueB);
			break;
#pragma endregion
#pragma region RES 6, n
		case 0xB7:
			A &= 0xBF;
			break;
		case 0xB0:
			BC.bytes.H &= 0xBF;
			break;
		case 0xB1:
			BC.bytes.L &= 0xBF;
			break;
		case 0xB2:
			DE.bytes.H &= 0xBF;
			break;
		case 0xB3:
			DE.bytes.L &= 0xBF;
			break;
		case 0xB4:
			HL.bytes.H &= 0xBF;
			break;
		case 0xB5:
			HL.bytes.L &= 0xBF;
			break;
		case 0xB6:
			immValueB = MemoryRead(HL.word) & 0xBF;

			MemoryWrite(HL.word, immValueB);
			break;
#pragma endregion
#pragma region RES 7, n
		case 0xBF:
			A &= 0x7F;
			break;
		case 0xB8:
			BC.bytes.H &= 0x7F;
			break;
		case 0xB9:
			BC.bytes.L &= 0x7F;
			break;
		case 0xBA:
			DE.bytes.H &= 0x7F;
			break;
		case 0xBB:
			DE.bytes.L &= 0x7F;
			break;
		case 0xBC:
			HL.bytes.H &= 0x7F;
			break;
		case 0xBD:
			HL.bytes.L &= 0x7F;
			break;
		case 0xBE:
			immValueB = MemoryRead(HL.word) & 0x7F;

			MemoryWrite(HL.word, immValueB);
			break;
#pragma endregion
		}

		opcode |= 0xCB00;

		break;
#pragma endregion

	default:
		break;
	}
	INT.Step(*this);
}

BYTE GameBoy::CPU::MemoryRead(WORD addr)
{
	BYTE value = MMU.Read(addr);
	SYNC_WITH_CPU(4);

	return value;
}

WORD GameBoy::CPU::MemoryReadWord(WORD addr)
{
	WORD value = MemoryRead(addr);
	value |= MemoryRead(addr + 1) << 8;

	return value;
}

void GameBoy::CPU::MemoryWrite(WORD addr, BYTE value)
{
	MMU.Write(addr, value);
	SYNC_WITH_CPU(4);
}

void GameBoy::CPU::MemoryWriteWord(WORD addr, WORD value)
{
	MemoryWrite(addr, value & 0xFF);
	MemoryWrite(addr + 1, (value & 0xFF00) >> 8);
}

void GameBoy::CPU::INTJump(WORD address)
{
	PC = address;

	SYNC_WITH_CPU(8);
}

void GameBoy::CPU::StackPushByte(BYTE value)
{
	SP--;
	MemoryWrite(SP, value);
}

void GameBoy::CPU::StackPushWord(WORD value)
{
	StackPushByte((value >> 8) & 0xFF);
	StackPushByte(value & 0xFF);

	SYNC_WITH_CPU(4);
}

BYTE GameBoy::CPU::StackPopByte()
{
	BYTE result = MemoryRead(SP);
	SP++;

	return result;
}

WORD GameBoy::CPU::StackPopWord()
{
	WORD result = StackPopByte();
	result |= StackPopByte() << 8;

	return result;
}