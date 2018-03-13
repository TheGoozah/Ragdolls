//--------------------------------------------------------------------------------------
// ForceFieldObject - Used to create a force field and maintain it
// Created by Matthieu Delaere
//--------------------------------------------------------------------------------------
#include "ForceFieldObject.h"
#include "../../../OverlordEngine/OverlordComponents.h"
#include "../../../OverlordEngine/Scenegraph/GameScene.h"

#include "../Managers/GameDirector.h"

ForceFieldObject::ForceFieldObject(GameDirector* pOwnerDirector, PhysicsGroup groupID, 
								   const NxVec3& sizeShape, const NxVec3& position):
	m_pOwnerDirector(pOwnerDirector),
	m_SizeShape(sizeShape),
	m_Position(position),
	m_groupID(groupID),
	m_pLinearKernel(nullptr), m_pForceField(nullptr),
	m_fCurrentLifeTime(0.0f), m_fMaximumLifeTime(4.5f),
	m_Force(NxVec3(0.0f, 45.0f, 0.0f))
{
}

ForceFieldObject::~ForceFieldObject(void)
{
	ReleaseResources();
}

NxForceField* ForceFieldObject::CreateForceField()
{
	//Safety checks
	if(m_pOwnerDirector == nullptr)
		return nullptr;
	GameScene* pOwnerScene = m_pOwnerDirector->GetOwnerScene();
	if(pOwnerScene == nullptr)
		return nullptr;
	NxScene* pPhysicsScene = pOwnerScene->GetPhysicsScene();
	if(pPhysicsScene == nullptr)
		return nullptr;

	//Create the force field kernel
	NxForceFieldLinearKernelDesc linearKernelDesc;
	linearKernelDesc.setToDefault();
	linearKernelDesc.constant = m_Force;
	m_pLinearKernel = pPhysicsScene->createForceFieldLinearKernel(linearKernelDesc);

	//A box force field descriptor
	NxBoxForceFieldShapeDesc boxDesc;
	boxDesc.setToDefault();
	boxDesc.dimensions.set(m_SizeShape.x, m_SizeShape.y, m_SizeShape.z);
	boxDesc.pose.t.set(m_Position.x, m_Position.y, m_Position.z);

	//The force field descriptor
	NxForceFieldDesc fieldDesc;
	fieldDesc.setToDefault();
	fieldDesc.kernel = m_pLinearKernel;
	fieldDesc.group = m_groupID;
	fieldDesc.includeGroupShapes.push_back(&boxDesc);
	fieldDesc.rigidBodyType = NX_FF_TYPE_GRAVITATIONAL;

	//Create the force field
	//We need to add our shape to the special include group so our box moves with the forcefield and 
	//it can't be included by other force fields
	m_pForceField = pPhysicsScene->createForceField(fieldDesc);

	return m_pForceField;
}

void ForceFieldObject::Update(GameContext& context)
{
	//Hold lifetime
	m_fCurrentLifeTime += context.GameTime.ElapsedSeconds();
}

bool ForceFieldObject::ReachedEndLifeTime() const
{
	if(m_fCurrentLifeTime >= m_fMaximumLifeTime)
		return true;
	else
		return false;
}

void ForceFieldObject::ReleaseResources()
{
	//Safety checks
	if(m_pOwnerDirector == nullptr)
		return;
	GameScene* pOwnerScene = m_pOwnerDirector->GetOwnerScene();
	if(pOwnerScene == nullptr)
		return;
	NxScene* pPhysicsScene = pOwnerScene->GetPhysicsScene();
	if(pPhysicsScene == nullptr)
		return;

	//Release the force field
	pPhysicsScene->releaseForceField(*m_pForceField);
	//Release the force kernel
	pPhysicsScene->releaseForceFieldLinearKernel(*m_pLinearKernel);
}