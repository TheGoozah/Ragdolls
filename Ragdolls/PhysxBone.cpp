//--------------------------------------------------------------------------------------
// PhysxBone representation used for the ragdolls in OverlordEngine
// Created by Matthieu Delaere
//--------------------------------------------------------------------------------------
#include "PhysxBone.h"
#include "../Ragdolls/PhysxSkeleton.h"
#include "../../../OverlordEngine/Managers/PhysicsManager.h"
#include "../../../OverlordEngine/Diagnostics/Logger.h"

PhysxBone::PhysxBone(NxScene* pScene, const PhysxBoneLayout& boneLayout, PhysxSkeleton* pOwnerSkeleton):
	m_boneLayout(boneLayout),
	m_pActor(nullptr),
	m_pBone(nullptr),
	m_iBoneIndex(-1),
	m_pPhysicsScene(pScene),
	m_pOwnerSkeleton(pOwnerSkeleton),
	debugValue(0.123456789f)
{
	D3DXMatrixIdentity(&m_matTotalOffset);
	D3DXMatrixIdentity(&m_matModelWorldSpace);
	D3DXMatrixIdentity(&m_matActorWorldSpace);
	D3DXMatrixIdentity(&m_matActorToModelTransform);
}

PhysxBone::~PhysxBone(void)
{
	if(m_pActor)
		m_pPhysicsScene->releaseActor(*m_pActor);
}

void PhysxBone::Initiliaze(MeshFilter* pMeshFilter, PhysicsGroup group)
{
	//Map the bone
	MapToBone(pMeshFilter);
	//Create actor and set it on the correct position
	CreatePhysxBone(pMeshFilter, group);
}

void PhysxBone::MapToBone(MeshFilter* pMeshFilter)
{
	//Find the bone we want to map to
	for(auto bone : pMeshFilter->GetSkeleton())
	{
		if(m_boneLayout.name == bone.Name)
		{
			m_pBone = &bone;
			break;
		}
	}
	//Check if we found our bone, else there is a mistake with the input
	ASSERT(m_pBone!=nullptr, _T("PhysxBone NAME INCORRECT! PhysxBone can not be mapped to a Bone in the Model!"));

	if(m_pBone)
	{
		//Store the index
		m_iBoneIndex = m_pBone->Index;

		//Create matrix that converts the offsetOrientation from PhysX to Max axis
		//PhysX: 0,0,0 rotation == capsule pointing up
		//Max: 0,0,0 rotation == capsule pointing right
		D3DXMATRIX matOrientationMaxToPhysx;
		D3DXMatrixIdentity(&matOrientationMaxToPhysx);
		D3DXMatrixRotationYawPitchRoll(&matOrientationMaxToPhysx, 0.0f, 0.0f, (float)D3DXToRadian(-90.0f));

		//So the total offset equals rotating the bone like in max and offset it with the data from max
		m_matTotalOffset = matOrientationMaxToPhysx * m_pBone->Offset;

		//Calculate the position of the bone in worldspace (Bind-Pose)
		m_matActorWorldSpace = m_matTotalOffset * m_matModelWorldSpace;
	}
}

void PhysxBone::UpdateLeechMode(const D3DXMATRIX& matKeyTransform)
{
	//Calculate the position of the bone using following formula
	//boneOffset * boneAnimTransform * worldTransformModel
	m_matActorWorldSpace = m_matTotalOffset * matKeyTransform * m_matModelWorldSpace;

	NxMat34 nPos;
	PhysicsManager::GetInstance()->DMatToNMat(nPos, m_matActorWorldSpace);
	m_pActor->setGlobalPose(nPos);
}

void PhysxBone::UpdateSeedMode()
{
	//Transform the actor back in model space after the simul of PhysX
	//Get our actor position and convert it
	D3DXMATRIX actorWorldSpace, totalOffsetInverse, modelWorldSpaceInverse;
	NxMat34 actorPhysxWorldSpace =  m_pActor->getGlobalPose();

	PhysicsManager::GetInstance()->NMatToDMat(actorWorldSpace, actorPhysxWorldSpace);
	//Get the inverse matrices of the matrices we used to position our actor
	D3DXMatrixInverse(&totalOffsetInverse, NULL, &m_matTotalOffset);
	D3DXMatrixInverse(&modelWorldSpaceInverse, NULL, &m_matModelWorldSpace);

	//Calculate our final position by using inverse matrix of our offset
	m_matActorToModelTransform = totalOffsetInverse * actorWorldSpace * modelWorldSpaceInverse;
}

void PhysxBone::CreatePhysxBone(MeshFilter* pMeshFilter, PhysicsGroup group)
{
	//Creates the bone using the information we know when we mapped the bone
	//Also taking into account which shape we want
	if(m_boneLayout.shapeType == RagdollShapeType::capsule)
	{
		//Create the capsule shape desc
		NxCapsuleShapeDesc capsuleDesc;
		capsuleDesc.setToDefault();
		capsuleDesc.height = m_boneLayout.height;
		capsuleDesc.radius = m_boneLayout.radius;
		capsuleDesc.localPose.t = NxVec3(0, capsuleDesc.radius + 0.5f * capsuleDesc.height, 0);
		capsuleDesc.group = group;

		//Create body so the actor is dynamic
		NxBodyDesc bodyDesc;
		bodyDesc.setToDefault();
		bodyDesc.angularDamping = 0.75f;
		bodyDesc.linearVelocity = NxVec3(0,0,0);

		//Create the actor
		NxActorDesc actorDesc;
		actorDesc.shapes.pushBack(&capsuleDesc);
		actorDesc.body = &bodyDesc;
		actorDesc.density = 10.0f; 
		//Important to set a density if we have a body to succesfully create an actor with a body

		//Position the actor to the correct place
		NxMat34 nPos;
		PhysicsManager::GetInstance()->DMatToNMat(nPos, m_matActorWorldSpace);
		actorDesc.globalPose = nPos;

		//Create the actor
		m_pActor = m_pPhysicsScene->createActor(actorDesc);
		
		if(!m_pActor)
			Logger::Log(_T("Error creating actor"), LogLevel::Error);

		m_pActor->userData = this;

		//Default set our actor to be kinematic
		m_pActor->raiseBodyFlag(NX_BF_KINEMATIC);
		m_pActor->raiseActorFlag(NX_AF_DISABLE_COLLISION);
	}
	else if(m_boneLayout.shapeType == RagdollShapeType::sphere)
	{	
		//Create the sphere shape desc
		NxSphereShapeDesc sphereDesc;
		sphereDesc.radius = m_boneLayout.radius;
		sphereDesc.localPose.t = NxVec3(0, m_boneLayout.radius, 0);
		sphereDesc.group = group;

		//Create body so the actor is dynamic
		NxBodyDesc bodyDesc;
		bodyDesc.setToDefault();
		bodyDesc.angularDamping = 0.75f;
		bodyDesc.linearVelocity = NxVec3(0,0,0);

		NxActorDesc actorDesc;
		actorDesc.shapes.pushBack(&sphereDesc);
		actorDesc.body = &bodyDesc;
		actorDesc.density = 10.0f; 
		//Important to set a density if we have a body to succesfully create an actor with a body

		//Position the actor to the correct place
		NxMat34 nPos;
		PhysicsManager::GetInstance()->DMatToNMat(nPos, m_matActorWorldSpace);
		actorDesc.globalPose = nPos;

		//Create the actor
		m_pActor = m_pPhysicsScene->createActor(actorDesc);

		if(!m_pActor)
			Logger::Log(_T("Error creating actor"), LogLevel::Error);

		m_pActor->userData = this;

		//Default set our actor to be kinematic
		m_pActor->raiseBodyFlag(NX_BF_KINEMATIC);
		m_pActor->raiseActorFlag(NX_AF_DISABLE_COLLISION);
	}
}