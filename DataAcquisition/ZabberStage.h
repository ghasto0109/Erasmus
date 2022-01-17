#ifndef ZABBER_STAGE_H_
#define ZABBER_STAGE_H_

#include <objcpp/property.h>

class ZabberStage
{
public:
	ZabberStage();

	void accept(property_visitor &visit);

	// properties
	property<int> Unit;
	property<int> Min, Max, Step;
	property<int> Position;

	operation MoveForward, MoveReverse;

private:
	static bool _initialized;

	void moveForward();
	void moveReverse();
/*
	static bool init(const char *portName);

	ZabberStage();

	property<int> Unit;

	void home();
	void stop();
	void moveAbsolute(int position);

	int getStageSpeed() { return _stageSpeed; }
	void setStageSpeed(int stageSpeed) { _stageSpeed = stageSpeed; }

private:
	int _unit;
	int _stageSpeed;
*/
};

#endif // ZABBER_STAGE_H_