#include "core/i_inventory_component.h"
#include "core/i_health_component.h"
#include "core/pickup_collision_component.h"
#include "platform/i_platform.h"
#include "platform/auto_id.h"

using platform::AutoId;

PickupCollisionComponent::PickupCollisionComponent()
    : CollisionComponent()
    , mPickupContent( 0 )
{

}

void PickupCollisionComponent::Collide(Actor& Other)
{
    //TODO: action! this should not make this fun here
    Opt<IInventoryComponent> inventoryC = Other.Get<IInventoryComponent>();
    if (inventoryC.IsValid())
    {
        inventoryC->DropItemType( Item::Weapon );
        inventoryC->AddItem( mPickupContent );
    }
    mActor->Get<IHealthComponent>()->SetHP(0);
}

void PickupCollisionComponent::SetPickupContent(int32_t PickupContent)
{
    mPickupContent=PickupContent;
}

void PickupCollisionComponentLoader::BindValues()
{
    Bind("radius",func_double(&PickupCollisionComponent::SetRadius));
    std::string istr;
    if( Json::GetStr( (*mSetters)["class"], istr))
    {
        Bind<CollisionClass::Type>(&PickupCollisionComponent::SetCollisionClass,mCollisionClass(AutoId(istr)));
    }
    istr.clear();
    if( Json::GetStr( (*mSetters)["content"], istr))
    {
        Bind<int32_t>(&PickupCollisionComponent::SetPickupContent,AutoId(istr));
    }
}

PickupCollisionComponentLoader::PickupCollisionComponentLoader()
    : mCollisionClass(CollisionClass::Get())
{
}