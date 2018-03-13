#ifndef PHYSXBONE_H_INCLUDED_
#define PHYSXBONE_H_INCLUDED_
//--------------------------------------------------------------------------------------
// PhysxBone representation used for the ragdolls in OverlordEngine
// Created by Matthieu Delaere
//--------------------------------------------------------------------------------------
#pragma once
#include "../../../OverlordEngine/Helpers/stdafx.h"
#include "../../../OverlordEngine/Helpers/D3DUtil.h"
#include "../../../OverlordEngine/OverlordComponents.h"
#include "../Ragdolls/RagdollHelper.h"
#include <algorithm>
#include <vector>

class PhysxSkeleton;

class PhysxBone final
{
public:
	//Constructor and Destructor
	PhysxBone(NxScene* pScene, const PhysxBoneLayout& boneLayout, PhysxSkeleton* pOwnerSkeleton);
	~PhysxBone(void);

	//METHODS
	//Creates and maps the bone
	void Initiliaze(MeshFilter* pMeshFilter, PhysicsGroup group);
	//Update Bone
	void UpdateLeechMode(const D3DXMATRIX& matKeyTransform);
	void UpdateSeedMode();

	//GETTERS
	NxActor* GetActor() const {return m_pActor;};
	const int GetIndex() const {return m_iBoneIndex;};
	const PhysxBoneLayout GetBoneLayout() const {return m_boneLayout;};
	const D3DXMATRIX GetActorInModelSpaceTransform() const {return m_matActorToModelTransform;};
	const D3DXMATRIX GetActorInWorldSpaceTransform() const {return m_matModelWorldSpace;};
	const D3DXMATRIX GetActorOffset() const {return m_matTotalOffset;};
	PhysxSkeleton* GetOwnerSkeleton() const {return m_pOwnerSkeleton;};
	float GetDebugValue() const {return debugValue;};

	//SETTERS
	void SetModelWorldTransform(const D3DXMATRIX& worldTransform){m_matModelWorldSpace = worldTransform;};
	void RaiseBodyFlag(NxBodyFlag flag){m_pActor->raiseBodyFlag(flag);};
	void ClearBodyFlag(NxBodyFlag flag){m_pActor->clearBodyFlag(flag);};
	void RaiseActorFlag(NxActorFlag flag){m_pActor->raiseActorFlag(flag);};
	void ClearActorFlag(NxActorFlag flag){m_pActor->clearActorFlag(flag);};

private:
	//DATAMEMBERS
	PhysxBoneLayout m_boneLayout; //Layout of the bone

	D3DXMATRIX m_matTotalOffset; //TotalOffset of the bone based on parents
	D3DXMATRIX m_matModelWorldSpace; //The world space position of our model we are linked to
	D3DXMATRIX m_matActorWorldSpace; //WorldSpace Position of the Actor itself

	NxActor* m_pActor; //The PhysX actor of this bone

	Bone* m_pBone; //The bone we mapped to
	int m_iBoneIndex; //The index of the bone we mapped to

	NxScene* m_pPhysicsScene; //Pointer to our PhysXScene
	PhysxSkeleton* m_pOwnerSkeleton; //Pointer to the Skeleton owning this bone

	D3DXMATRIX m_matActorToModelTransform; //The matrix holding the actor's position in model space

	float debugValue; //Temp used in contactreport to avoid calling non physxBone

	//METHODS
	void MapToBone(MeshFilter* pMeshFilter);
	void CreatePhysxBone(MeshFilter* pMeshFilter, PhysicsGroup group);

	//Operators
	// -------------------------
	// Disabling default copy constructor and default 
	// assignment operator.
	// -------------------------
	PhysxBone(const PhysxBone& yRef);									
	PhysxBone& operator=(const PhysxBone& yRef);
};
#endif