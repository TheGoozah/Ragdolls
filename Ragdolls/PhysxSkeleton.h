#ifndef PHYSXSKELETON_H_INCLUDED_
#define PHYSXSKELETON_H_INCLUDED_
//--------------------------------------------------------------------------------------
// PhysxSkeleton representation used for the ragdolls in OverlordEngine
// Created by Matthieu Delaere
//--------------------------------------------------------------------------------------
#pragma once
#include "../../../OverlordEngine/Helpers/stdafx.h"
#include "../../../OverlordEngine/Helpers/D3DUtil.h"
#include "../../../OverlordEngine/OverlordComponents.h"
#include "../Ragdolls/RagdollHelper.h"
#include "../Ragdolls/PhysxBone.h"
#include <vector>
#include <memory>

class PhysxSkeleton final
{
public:
	//Constructor and Destructor
	PhysxSkeleton(NxScene* pScene, PhysicsGroup group, PhysicsAnimator* ownerPhysicsAnimator);
	~PhysxSkeleton(void);

	//Methods
	//Adds a bone to the skeleton
	void AddBone(const PhysxBoneLayout& boneLayout);
	//Adds a joint layout to the skeleton so we can create our joints when needed
	void AddJoint(const PhysxJointLayout& jointLayout);
	//Creates and maps the bones of the skeleton
	void Initiliaze(MeshFilter* pMeshFilter);
	//Updates the skeleton (all the bones)
	void UpdateLeechMode(GameContext& context);
	void UpdateSeedMode(GameContext& context);
	//Creates all joints
	void CreateJoints();
	//Releases all joints
	void ReleaseJoints();

	//Getters
	//Seeds bone transforms based on PhysX actors
	vector<D3DXMATRIX> SeedBoneTransforms() const {return m_BonePhysicsTransforms;};
	//Returns all the PhysxBones
	vector<PhysxBone*> GetPhysxBones() const {return m_vpPhysxBones;};
	//Searches for the PhysxBone mapped based on the received layout
	PhysxBone* GetPhysxBone(const PhysxBoneLayout& boneLayout) const;
	//Return the PhysxAnimator owning this skeleton
	PhysicsAnimator* GetOwnerPhysxAnimator() const {return m_pOwnerPhysicsAnimator;};
	//Returns all actors of this skeleton
	vector<NxActor*> GetBoneActors() const;
	//Returns the root bone's actor. == First bone in hierarchy
	NxActor* GetRootBoneActor() const;

	//Setters
	//sets the bone transforms
	void FeedBoneTransforms(vector<D3DXMATRIX> boneTransforms){m_BoneOriginalTransforms = boneTransforms;};
	//Sets the worldTransform of the object we resemble. Needed for all bones of this skeleton.
	void SetWorldTransform(const D3DXMATRIX& worldTransform){m_matWorldTransform = worldTransform;};

private:
	//Datamembers
	vector<PhysxBone*> m_vpPhysxBones;
	vector<PhysxJointLayout> m_vJointLayouts;
	vector<NxSphericalJoint*> m_vpSphericalJoints;
	vector<NxRevoluteJoint*> m_vpRevoluteJoints;

	vector<D3DXMATRIX> m_BoneOriginalTransforms;
	vector<D3DXMATRIX> m_BonePhysicsTransforms;
	D3DXMATRIX m_matWorldTransform;

	NxScene* m_pPhysicsScene;
	PhysicsGroup m_nxPhysxGroup;

	PhysicsAnimator* m_pOwnerPhysicsAnimator;

	//Methods
	void CreateSphericalJoint(PhysxBone* bone1, PhysxBone* bone2, const NxVec3& globalAnchor, const NxVec3& globalAxis);
	void CreateRevoluteJoint(PhysxBone* bone1, PhysxBone* bone2, const NxVec3& globalAnchor, const NxVec3& globalAxis);

	//Operators
	// -------------------------
	// Disabling default copy constructor and default 
	// assignment operator.
	// -------------------------
	PhysxSkeleton(const PhysxSkeleton& yRef);									
	PhysxSkeleton& operator=(const PhysxSkeleton& yRef);
};
#endif