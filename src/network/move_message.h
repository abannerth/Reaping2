#ifndef INCLUDED_NETWORK_MOVE_MESSAGE_H
#define INCLUDED_NETWORK_MOVE_MESSAGE_H

#include "network/message.h"
namespace network {

    class MoveMessage: public Message
    {
        friend class ::boost::serialization::access;
    public:
        DEFINE_MESSAGE_BASE(MoveMessage)
        int32_t mActorGUID;
        int32_t mHeadingModifier;
        int32_t mSpeed;
        int32_t mPercent;
        int32_t mFlat;
        bool mMoving;
        
        MoveMessage()
            : mActorGUID(0)
            , mHeadingModifier(0)
            , mSpeed(0)
            , mPercent(0)
            , mFlat(0)
            , mMoving(false)
        {
        }
        template<class Archive>
        void serialize(Archive& ar, const unsigned int version);
        bool operator==(MoveMessage const& other)
        {
            return mActorGUID==other.mActorGUID
                && mHeadingModifier==other.mHeadingModifier
                && mSpeed==other.mSpeed
                && mPercent==other.mPercent
                && mFlat==other.mFlat
                && mMoving==other.mMoving;
        }
    };

    template<class Archive>
    void MoveMessage::serialize(Archive& ar, const unsigned int version)
    {
        ar & boost::serialization::base_object<Message>(*this);
        ar & mActorGUID;
        ar & mHeadingModifier;
        ar & mSpeed;
        ar & mPercent;
        ar & mFlat;
        ar & mMoving;
    }

} // namespace network
#endif//INCLUDED_NETWORK_MOVE_MESSAGE_H
