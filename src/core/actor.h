#ifndef INCLUDED_CORE_ACTOR_H
#define INCLUDED_CORE_ACTOR_H

struct CollisionClass
{
	enum Type{
		No_Collision,
		Projectile,
		Creep,
		Mine,
		Player,
		Num_Classes,
	};
};
class Actor;
typedef boost::intrusive::list<Actor> ActorList;
class Actor : public boost::intrusive::list_base_hook<>
{
public:
	boost::intrusive::list_member_hook<> mAllActorHook;
protected:
	enum {
		HP,
		DAMAGE,
		X,
		Y,
		ORIENTATION,	// merre nez
		RADIUS,
		HEADING,		// merre megy
		SPEED,
		SPEED_X,
		SPEED_Y,
		COLLISION_CLASS,
		GUID,			// todo: MakeGuid()
		TYPE_ID,
		ACTION_ID,
		ACTION_STATE,
		NUM_FIELDS,
	};
	union field_t
	{
		int32_t i;
		double d;
	};

	field_t mFields[NUM_FIELDS];
	std::auto_ptr<Controller> mController;

	void UpdateProjections()
	{
		const double spd=GetSpeed();
		const double h=GetHeading();
		const double c=cos(h);
		const double s=sin(h);
		mFields[SPEED_X].d=c*spd;
		mFields[SPEED_Y].d=s*spd;
	}
public:
	Actor();
	virtual ~Actor(){}
	virtual void Collide(double Seconds, ActorList& Actors);
	virtual void Update(double Seconds);

	void SetController(std::auto_ptr<Controller> Control);

	double GetX()const{return mFields[X].d;}
	double GetY()const{return mFields[Y].d;}
	double GetRadius()const{return mFields[RADIUS].d;}
	double GetSpeed()const{return mFields[SPEED].d;}
	double GetHeading()const{return mFields[HEADING].d;}
	double GetOrientation()const{return mFields[ORIENTATION].d;}
	int32_t GetTypeId()const{return mFields[TYPE_ID].i;}
	int32_t GetActionId()const{return mFields[ACTION_ID].i;}
	int32_t GetActionState()const{return (int32_t)(mFields[ACTION_STATE].d);}
	int32_t GetHP()const{return mFields[HP].i;}
	int32_t GetGUID()const{return mFields[GUID].i;}
	CollisionClass::Type GetCC()const{return CollisionClass::Type(mFields[COLLISION_CLASS].i);}

	void SetSpeed(double Speed)
	{
		mFields[SPEED].d=Speed;
		UpdateProjections();
	}
	void SetHeading(double Heading)
	{
		mFields[HEADING].d=Heading;
		UpdateProjections();
	}
	void SetOrientation(double Ori)
	{
		mFields[ORIENTATION].d=Ori;
	}
	void SetActionId(ActionHolder::ActionType ActionId, int32_t Position, bool Activate=true)
	{
		if(ActionId<ActionHolder::MOVE||ActionId>=ActionHolder::NUM_FIELDS)return;
		if(Position==-1)return;
		if (Activate)
		{
			mFields[ACTION_ID].i|=ActionId<<(Position*8);
		}
		else
		{
			mFields[ACTION_ID].i&=~(0xFF<<(Position*8));
		}
	}
	bool HasAction(ActionHolder::ActionType ActionId, int32_t& Position)
	{
		Position=-1;
		if(ActionId<ActionHolder::MOVE||ActionId>=ActionHolder::NUM_FIELDS)return false;
		int32_t gap=-1;
		int action = mFields[ACTION_ID].i;
		for(size_t i=0;i<4;++i)
		{
			if (gap==-1&&(action&0xFF)==0)
			{
				gap=i;
			}
			if ((action&0xFF)==ActionId)
			{
				Position=i;
				return true;
			}
			action>>=8;
		}
		Position=gap;
		return false;
	}
};

typedef boost::intrusive::member_hook< Actor, boost::intrusive::list_member_hook<>, &Actor::mAllActorHook> AllActorOption_t;
typedef boost::intrusive::list<Actor, AllActorOption_t> AllActorInSceneList;


#endif//INCLUDED_CORE_ACTOR_H