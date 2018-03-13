//--------------------------------------------------------------------------------------
// PhysxSkeleton representation used for the ragdolls in OverlordEngine
// Created by Matthieu Delaere
//--------------------------------------------------------------------------------------
#include "PhysxSkeleton.h"
#include "../Ragdolls/PhysicsAnimator.h"
#include "../../../OverlordEngine/Managers/PhysicsManager.h"
#include "../../../OverlordEngine/Diagnostics/Logger.h"

PhysxSkeleton::PhysxSkeleton(NxScene* pScene, PhysicsGroup group,  PhysicsAnimator* ownerPhysicsAnimator):
	m_pPhysicsScene(pScene),
	m_nxPhysxGroup(group),
	m_pOwnerPhysicsAnimator(ownerPhysicsAnimator)
{
	///Make sure our worldTransform is initialized is identity so it won't be put
	//in the wrong place if no concrete worldtransform is given allready
	D3DXMatrixIdentity(&m_matWorldTransform);

	//Fill the transform vectors with identity matrices for the amount of bones present.
	//We need to do this to ensure if someone would call this vector before we did any 
	//any calculations. The amount of bones can't be fetched here directly, but we can
	//ask for the transforms of the PhysxAnimator because it uses the same init principle
	D3DXMATRIX identityMatrix;
	D3DXMatrixIdentity(&identityMatrix);
	if(m_pOwnerPhysicsAnimator != nullptr)
	{
		for(UINT i=0; i<m_pOwnerPhysicsAnimator->SeedBoneTransforms().size(); ++i)
		{
			m_BoneOriginalTransforms.push_back(identityMatrix);
			m_BonePhysicsTransforms.push_back(identityMatrix);
		}
	}
}

PhysxSkeleton::~PhysxSkeleton(void)
{
	//Release active joints when deleting this object
	ReleaseJoints();

	for(auto physxBone : m_vpPhysxBones)
	{
		SafeDelete(physxBone);
	}
	m_vpPhysxBones.clear();

	m_vJointLayouts.clear();
}

void PhysxSkeleton::AddBone(const PhysxBoneLayout& boneLayout)
{
	if(m_pPhysicsScene)
		m_vpPhysxBones.push_back(new PhysxBone(m_pPhysicsScene, boneLayout, this));
}

PhysxBone* PhysxSkeleton::GetPhysxBone(const PhysxBoneLayout& boneLayout) const
{
	for(auto physxBone : m_vpPhysxBones)
	{
		if(physxBone->GetBoneLayout().name == boneLayout.name)
			return physxBone;
	}
	//If we found no bone matching the layout, there is a mistake with the input
	ASSERT(true, _T("PhysxBone does not exist!"));
	return nullptr;
}

vector<NxActor*> PhysxSkeleton::GetBoneActors() const
{
	vector<NxActor*> vBoneActors;
	for(auto bone : m_vpPhysxBones)
	{
		vBoneActors.push_back(bone->GetActor());
	}
	return vBoneActors;
}

void PhysxSkeleton::AddJoint(const PhysxJointLayout& jointLayout)
{
	m_vJointLayouts.push_back(jointLayout);
}

void PhysxSkeleton::Initiliaze(MeshFilter* pMeshFilter)
{
	//First check if all the input is correct
	//For n bones we need n-1 joints
	int amountPhysxBones = m_vpPhysxBones.size();
	int amountJoints = m_vJointLayouts.size() + 1;
	if(amountPhysxBones != amountJoints)
		ASSERT(true, _T("Can not construct correct skeleton! Mismatch amount of BoneLayouts and JointLayouts!"));

	//Creates and Maps all the bones
	for(auto physxBone : m_vpPhysxBones)
	{
		physxBone->Initiliaze(pMeshFilter, m_nxPhysxGroup);
	}

	//Get the root bone (first in vector) and lock if wanted
	PhysxBone* rootBone = m_vpPhysxBones.at(0);
	if(rootBone != nullptr)
	{
		rootBone->RaiseBodyFlag(NX_BF_FROZEN_POS_Z);
		//rootBone->RaiseBodyFlag(NX_BF_FROZEN_ROT_Z);
	}
}

void PhysxSkeleton::UpdateLeechMode(GameContext& context)
{
	//Updates all the bones
	for(auto physxBone : m_vpPhysxBones)
	{
		//Pass the world transform of model to the bone to do the necessary calculations
		physxBone->SetModelWorldTransform(m_matWorldTransform);
		//Get our index
		int i = physxBone->GetIndex();
		//Feed transform so we can calculate the actor's position
		physxBone->UpdateLeechMode(m_BoneOriginalTransforms.at(i));
	}
}

void PhysxSkeleton::UpdateSeedMode(GameContext& context)
{
	//Copy the original local transforms before adjusting them
	m_BonePhysicsTransforms = m_BoneOriginalTransforms;

	//Updates all the bones
	for(auto physxBone : m_vpPhysxBones)
	{
		//Pass the world transform of model to the bone to do the necessary calculations
		physxBone->SetModelWorldTransform(m_matWorldTransform);
		//Get our index
		int i = physxBone->GetIndex();
		//Calculate the new transform
		physxBone->UpdateSeedMode();
		//Store it by overriding copy of the original transform with the new transform
		m_BonePhysicsTransforms.at(i) = physxBone->GetActorInModelSpaceTransform();
	}
}

NxActor* PhysxSkeleton::GetRootBoneActor() const
{
	PhysxBone* rootBone = nullptr;
	NxActor* rootBoneActor = nullptr;

	if(!m_vpPhysxBones.empty())
		rootBone = m_vpPhysxBones.at(0);

	if(rootBone != nullptr)
		rootBoneActor = rootBone->GetActor();
	
	return rootBoneActor;
}

void PhysxSkeleton::CreateSphericalJoint(PhysxBone* bone1, PhysxBone* bone2, const NxVec3& globalAnchor, const NxVec3& globalAxis)
{
	NxSphericalJointDesc sphericalDesc;
	sphericalDesc.actor[0] = bone1->GetActor();
	sphericalDesc.actor[1] = bone2->GetActor();
	sphericalDesc.setGlobalAnchor(globalAnchor);
	sphericalDesc.setGlobalAxis(globalAxis);

	sphericalDesc.flags |= NX_SJF_TWIST_LIMIT_ENABLED;
	sphericalDesc.twistLimit.low.value = -(NxReal)0.025f*NxPi;
	sphericalDesc.twistLimit.low.hardness = 0.5f;
	sphericalDesc.twistLimit.low.restitution = 0.5f;
	sphericalDesc.twistLimit.high.value = (NxReal)0.025f*NxPi;
	sphericalDesc.twistLimit.high.hardness = 0.5f;
	sphericalDesc.twistLimit.high.restitution = 0.5f;

	sphericalDesc.flags |= NX_SJF_SWING_LIMIT_ENABLED;
	sphericalDesc.swingLimit.value = (NxReal)0.25f*NxPi;
	sphericalDesc.swingLimit.hardness = 0.5f;
	sphericalDesc.swingLimit.restitution = 0.5f;

	sphericalDesc.flags |= NX_SJF_TWIST_SPRING_ENABLED;
	sphericalDesc.twistSpring.spring = 0.5f;
	sphericalDesc.twistSpring.damper = 1.0f;

	sphericalDesc.flags |= NX_SJF_SWING_SPRING_ENABLED;
	sphericalDesc.swingSpring.spring = 0.5f;
	sphericalDesc.swingSpring.damper = 1.0f;

	//Joint projection is a method for correcting large joint errors.
	//NX_JPM_POINT_MINDIST : linear only minimum distance projection  
	sphericalDesc.projectionDistance = (NxReal)0.15f;
	sphericalDesc.projectionMode = NX_JPM_POINT_MINDIST;

	NxSphericalJoint* sphericalJoint = dynamic_cast<NxSphericalJoint*>(m_pPhysicsScene->createJoint(sphericalDesc));
	m_vpSphericalJoints.push_back(sphericalJoint);
}

void PhysxSkeleton::CreateRevoluteJoint(PhysxBone* bone1, PhysxBone* bone2, const NxVec3& globalAnchor, const NxVec3& globalAxis)
{
	NxRevoluteJointDesc revoluteDesc;
	revoluteDesc.actor[0] = bone1->GetActor();
	revoluteDesc.actor[1] = bone2->GetActor();
	revoluteDesc.setGlobalAnchor(globalAnchor);
	revoluteDesc.setGlobalAxis(globalAxis);

	/*NxJointLimitDesc limitLowDesc;
	limitLowDesc.value = 0.0f  * (NxPi/180.0f);
	revoluteDesc.limit.low = limitLowDesc;

	NxJointLimitDesc limitHighDesc;
	limitHighDesc.value = 90.0f * (NxPi/180.0f);
	revoluteDesc.limit.high = limitHighDesc;*/

	NxRevoluteJoint* revoluteJoint = dynamic_cast<NxRevoluteJoint*>(m_pPhysicsScene->createJoint(revoluteDesc));
	m_vpRevoluteJoints.push_back(revoluteJoint);
}

void PhysxSkeleton::CreateJoints()
{
	//For all JointLayouts, create the proper joints
	for(auto jointLayout : m_vJointLayouts)
	{
		if(jointLayout.jointType == JointType::spherical)
		{
			//Find the globalAnchor
			NxVec3 globalAnchor;
			if(jointLayout.anchorBone == JointBone::PhysxBone1)
				globalAnchor = jointLayout.pBone1->GetActor()->getGlobalPosition();
			else if(jointLayout.anchorBone == JointBone::PhysxBone2)
				globalAnchor = jointLayout.pBone2->GetActor()->getGlobalPosition();

			//CreateJoint
			CreateSphericalJoint(jointLayout.pBone1, jointLayout.pBone2, globalAnchor, jointLayout.axisOrientation);
		}
		else if(jointLayout.jointType == JointType::revolute)
		{
			//Find the globalAnchor
			NxVec3 globalAnchor;
			if(jointLayout.anchorBone == JointBone::PhysxBone1)
				globalAnchor = jointLayout.pBone1->GetActor()->getGlobalPosition();
			else if(jointLayout.anchorBone == JointBone::PhysxBone2)
				globalAnchor = jointLayout.pBone2->GetActor()->getGlobalPosition();

			//CreateJoint
			CreateRevoluteJoint(jointLayout.pBone1, jointLayout.pBone2, globalAnchor, jointLayout.axisOrientation);
		}
	}
}

void PhysxSkeleton::ReleaseJoints()
{
	for(auto sphericalJoint : m_vpSphericalJoints)
	{
		if(sphericalJoint != nullptr)
			m_pPhysicsScene->releaseJoint(*sphericalJoint);
	}
	for(auto revoluteJoint : m_vpRevoluteJoints)
	{
		if(revoluteJoint != nullptr)
			m_pPhysicsScene->releaseJoint(*revoluteJoint);
	}

	//clear vectors
	m_vpSphericalJoints.clear();
	m_vpRevoluteJoints.clear();
}