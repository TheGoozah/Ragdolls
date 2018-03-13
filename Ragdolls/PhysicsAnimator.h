#ifndef PHYSICSANIMATOR_H_INCLUDED_
#define PHYSICSANIMATOR_H_INCLUDED_
//--------------------------------------------------------------------------------------
// PhysxAnimator used to control the ragdoll in OverlordEngine
// Created by Matthieu Delaere
//--------------------------------------------------------------------------------------
#pragma once
#include "../../../OverlordEngine/Helpers/stdafx.h"
#include "../../../OverlordEngine/Helpers/D3DUtil.h"
#include "../../../OverlordEngine/Helpers/GeneralStructs.h"
#include "../../../OverlordEngine/OverlordComponents.h"

#include "../../SZS_Tools/PugiXML/pugixml.hpp"

#include "RagdollHelper.h"
#include "PhysxSkeleton.h"
#include <vector>

class PhysicsAnimator final
{
public:
	PhysicsAnimator(NxScene* pScene, MeshFilter* pMeshFilter, ModelComponent* ownerModelComponent);
	~PhysicsAnimator(void);

	//METHODS
	//Creates the ragdoll skeleton
	void BuildPhysicsSkeleton(PhysicsGroup group);
	void BuildPhysicsSkeletonFromFile(PhysicsGroup group);

	//Calculate the bones transforms in LeechMode (DirectX model -> PhysX)
	void UpdateLeechMode(GameContext& context);
	//Calculate the bones transform in SeedMode (PhysX -> DirectX model)
	void UpdateSeedMode(GameContext& context);

	//SETTERS
	//Sets the bone transforms
	void FeedBoneTransforms(vector<D3DXMATRIX> boneTransforms){m_BoneOriginalTransforms = boneTransforms;};
	//Sets our state
	void SetCurrentState(RagdollState state);
	//Sets the worldTransform of our owner object (the object we resemble)
	void SetWorldTransform(const D3DXMATRIX& worldTransform);

	//GETTERS
	//Return the bone transforms
	vector<D3DXMATRIX> SeedBoneTransforms() const {return m_BonePhysicsTransforms;};
	//Get our current state
	const RagdollState GetCurrentState() const {return m_currentRagdollState;};
	//Returns the pointer of the modelcompenent owning this animator
	ModelComponent* GetOwnerModelComponent() const {return m_pOwnerModelComponent;};
	//Returns all actors of the skeleton used by this Animator
	PhysxSkeleton* GetSkeleton() const;
	//Returns the position of the root actor
	//NxVec3 GetRootActorPosition();

private:
	//DATAMEMBERS
	NxScene* m_pPhysicsScene;
	MeshFilter* m_pMeshFilter;
	vector<D3DXMATRIX> m_BoneOriginalTransforms;
	vector<D3DXMATRIX> m_BonePhysicsTransforms;
	D3DXMATRIX m_matWorldTransform;

	PhysxSkeleton* m_pPhysxSkeleton;
	RagdollState m_currentRagdollState;

	ModelComponent* m_pOwnerModelComponent;

	//METHODS
	void PrepareForLeech();
	void PrepareForSeed();

	// -------------------------
	// Disabling default copy constructor and default 
	// assignment operator.
	// -------------------------
	PhysicsAnimator(const PhysicsAnimator& yRef);									
	PhysicsAnimator& operator=(const PhysicsAnimator& yRef);
};
#endif