#include "i_core.h"

RandomController::RandomController():Controller()
	,mCounter(2)
	,mHeadingModifier(0)
{

}

void RandomController::SetActor(Actor* Obj)
{
	Controller::SetActor(Obj);
	if(!mActor)return;
	mActor->SetHeading(rand()%180);
	Action const& Act=ActionRepo::Get()(AutoId("move"));
	Act.Activate(*mActor);
}

void RandomController::Update( double Seconds )
{
	if(!mActor)return;
	mCounter+=Seconds;
	if (mCounter>2)
	{
		mCounter=0;
		mHeadingModifier=(rand()%10-5)*.33;

		mActor->SetSpeed((rand()%10)*.03);
	}
	mActor->SetHeading(mActor->GetHeading()+Seconds*mHeadingModifier);
	mActor->SetOrientation(mActor->GetHeading());
}

