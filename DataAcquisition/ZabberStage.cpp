#include "stdafx.h"
#include "ZabberStage.h"

extern "C"
{
	#include "pserial.h"
}

using namespace std;

const char *ZABBER_COM_PORT = "COM1";
const int CMD_MOVE_ABSOLUTE = 20;

bool ZabberStage::_initialized = false;

ZabberStage::ZabberStage() :
	Unit("Unit", 1), 
	Min("Min", 10000), 
	Max("Max", 130000), 
	Step("Step", 5000), 
	Position("Position", 70000), 

	MoveForward("Move Forward", this, &ZabberStage::moveForward), 
	MoveReverse("Move Reverse", this, &ZabberStage::moveReverse)
{
	// WARNING: not thread safe
	if (!_initialized)
	{
		PSERIAL_Initialize();
		if (!PSERIAL_Open(ZABBER_COM_PORT))
		{
			cout << "Failed to initialize Zabber stage: " << ZABBER_COM_PORT << endl;
			return;
		}

		_initialized = true;
	}

	Position.valueChanged += [&](int value)
	{
		//if (value > Max)
		//	Position = Max;
		//else if (value < Min)
		//	Position = Min;
		//else
		//{
		cout << "Zabber stage position: " << value << endl;
		PSERIAL_Send(Unit, CMD_MOVE_ABSOLUTE, Position);
		//}
	};
}

void ZabberStage::accept(property_visitor &visit)
{
	visit(Unit);
	visit(Min);
	visit(Max);
	visit(Step);
	visit(Position);

	visit(MoveForward);
	visit(MoveReverse);
}

void ZabberStage::moveForward()
{
	Position = Position + Step;
}

void ZabberStage::moveReverse()
{
	Position = Position - Step;
}

#if 0

void ZabberStage::home()
{
	PSERIAL_Send(_unit, 1, 0);
}

void ZabberStage::stop()
{
	PSERIAL_Send(_unit, 23, 0);
}

void ZabberStage::moveForward()
{
	PSERIAL_Send(Unit, 22, _stageSpeed);
}

void ZabberStage::MoveReverse()
{
	PSERIAL_Send(Unit, 22, -1 * _stageSpeed);
}

void ZabberStage::moveAbsolute(int position)
{
	PSERIAL_Send(_unit, 20, position);
}

#endif