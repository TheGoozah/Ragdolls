//--------------------------------------------------------------------------------------
// PhysxAnimator used to control the ragdoll in OverlordEngine
// Created by Matthieu Delaere
//--------------------------------------------------------------------------------------
#include "PhysicsAnimator.h"
#include "../../../OverlordEngine/Managers/PhysicsManager.h"
#include "../../../OverlordEngine/Diagnostics/Logger.h"

PhysicsAnimator::PhysicsAnimator(NxScene* pScene, MeshFilter* pMeshFilter, ModelComponent* ownerModelComponent):
	m_pPhysicsScene(pScene),
	m_pMeshFilter(pMeshFilter),
	m_pOwnerModelComponent(ownerModelComponent),
	m_pPhysxSkeleton(nullptr),
	m_currentRagdollState(RagdollState::LeechState)
{
	///Make sure our worldTransform is initialized is identity so it won't be put
	//in the wrong place if no concrete worldtransform is given allready
	D3DXMatrixIdentity(&m_matWorldTransform);

	//Fill the transform vectors with identity matrices for the amount of bones present.
	//We need to do this to ensure if someone would call this vector before we did any 
	//any calculations
	D3DXMATRIX identityMatrix;
	D3DXMatrixIdentity(&identityMatrix);
	if(m_pMeshFilter != nullptr)
	{
		for(UINT i=0; i < m_pMeshFilter->GetSkeleton().size(); ++i)
		{
			m_BoneOriginalTransforms.push_back(identityMatrix);
			m_BonePhysicsTransforms.push_back(identityMatrix);
		}
	}
}

PhysicsAnimator::~PhysicsAnimator(void)
{
	SafeDelete(m_pPhysxSkeleton);
}

void PhysicsAnimator::BuildPhysicsSkeletonFromFile(PhysicsGroup group)
{
	//TEST IS NOT ERROR PRONE... THIS IS FOR TESTING THE TOOL ONLY!!!!! USING AN APPROVED SETUP!
	if(m_pPhysicsScene)
	{
		//Create skeleton
		if(m_pPhysxSkeleton != nullptr)
			delete m_pPhysxSkeleton;
		m_pPhysxSkeleton = new PhysxSkeleton(m_pPhysicsScene, group, this);

		//---------------------------------------------------------
		//Create a document on the stack (RAII)
		pugi::xml_document doc;
		//Load a document and see if it has been found - path hardcoded for testing!
		pugi::xml_parse_result result = doc.load_file(_T("./SZS_Resources/Skeleton/GameSkeleton.xml"));

		//Root Node
		pugi::xml_node rs = doc.child(_T("RagdollSkeleton"));

		//---------------------------------------------------------
		//Load BoneLayouts
		pugi::xml_node boneLayouts = rs.child(_T("BoneLayouts"));

		vector<PhysxBoneLayout> vecBoneLayouts;
		for(pugi::xml_node node = boneLayouts.child(_T("Bone")); node != nullptr; node = node.next_sibling(_T("Bone")))
		{
			PhysxBoneLayout boneLayout;
			boneLayout.name = node.attribute(_T("name")).as_string();
			boneLayout.height = node.attribute(_T("height")).as_float();
			boneLayout.radius = node.attribute(_T("radius")).as_float();
			boneLayout.shapeType = (RagdollShapeType)node.attribute(_T("type")).as_int();

			m_pPhysxSkeleton->AddBone(boneLayout);
			vecBoneLayouts.push_back(boneLayout);
		}

		//---------------------------------------------------------
		//Load BoneJoints
		pugi::xml_node boneJoints = rs.child(_T("BoneJoints"));

		for(pugi::xml_node node = boneJoints.child(_T("Joint")); node != nullptr; node = node.next_sibling(_T("Joint")))
		{
			PhysxJointLayout jointLayout;
			jointLayout.jointType = (JointType)node.attribute(_T("type")).as_int();

			tstring bone1Name = node.attribute(_T("bone1")).as_string();
			tstring bone2Name = node.attribute(_T("bone2")).as_string();
			auto bone1Layout = *find_if(vecBoneLayouts.begin(), vecBoneLayouts.end(), [&] (PhysxBoneLayout boneLayout) { 
				return (boneLayout.name == bone1Name);});
			auto bone2Layout = *find_if(vecBoneLayouts.begin(), vecBoneLayouts.end(), [&] (PhysxBoneLayout boneLayout) { 
				return (boneLayout.name == bone2Name);});

			jointLayout.pBone1 = m_pPhysxSkeleton->GetPhysxBone(bone1Layout);
			jointLayout.pBone2 = m_pPhysxSkeleton->GetPhysxBone(bone2Layout);
			jointLayout.anchorBone = JointBone::PhysxBone2;

			NxVec3 axisOrietation;
			axisOrietation.x = node.attribute(_T("axis-x")).as_float();
			axisOrietation.y = node.attribute(_T("axis-y")).as_float();
			axisOrietation.z = node.attribute(_T("axis-z")).as_float();
			jointLayout.axisOrientation = axisOrietation;

			m_pPhysxSkeleton->AddJoint(jointLayout);
		}

		//---------------------------------------------------------
		//Build skeleton
		m_pPhysxSkeleton->Initiliaze(m_pMeshFilter);
		m_pPhysxSkeleton->CreateJoints();
	}
}

void PhysicsAnimator::BuildPhysicsSkeleton(PhysicsGroup group)
{
	if(m_pPhysicsScene)
	{
		//Create skeleton
		m_pPhysxSkeleton = new PhysxSkeleton(m_pPhysicsScene, group, this);

		//---------------------------------------------------------
		//Bone 1
		PhysxBoneLayout boneLayout1;
		boneLayout1.name = _T("Spine0");
		boneLayout1.height = 0.15f;
		boneLayout1.radius = 0.2f;
		boneLayout1.shapeType = RagdollShapeType::capsule;

		m_pPhysxSkeleton->AddBone(boneLayout1);

		//Bone 2
		PhysxBoneLayout boneLayout2;
		boneLayout2.name = _T("Spine1");
		boneLayout2.height = 0.025f;
		boneLayout2.radius = 0.45f;
		boneLayout2.shapeType = RagdollShapeType::capsule;

		m_pPhysxSkeleton->AddBone(boneLayout2);

		//Bone 3
		PhysxBoneLayout boneLayout3;
		boneLayout3.name = _T("Head");
		boneLayout3.radius = 0.65f;
		boneLayout3.shapeType = RagdollShapeType::sphere;

		m_pPhysxSkeleton->AddBone(boneLayout3);

		//Bone 4
		PhysxBoneLayout boneLayout4;
		boneLayout4.name = _T("RightUpperArm");
		boneLayout4.height = 0.35f;
		boneLayout4.radius = 0.15f;
		boneLayout4.shapeType = RagdollShapeType::capsule;

		m_pPhysxSkeleton->AddBone(boneLayout4);

		//Bone 5
		PhysxBoneLayout boneLayout5;
		boneLayout5.name = _T("RightLowerArm");
		boneLayout5.height = 0.35f;
		boneLayout5.radius = 0.15f;
		boneLayout5.shapeType = RagdollShapeType::capsule;

		m_pPhysxSkeleton->AddBone(boneLayout5);

		//Bone 6
		PhysxBoneLayout boneLayout6;
		boneLayout6.name = _T("LeftUpperArm");
		boneLayout6.height = 0.35f;
		boneLayout6.radius = 0.15f;
		boneLayout6.shapeType = RagdollShapeType::capsule;

		m_pPhysxSkeleton->AddBone(boneLayout6);

		//Bone 7
		PhysxBoneLayout boneLayout7;
		boneLayout7.name = _T("LeftLowerArm");
		boneLayout7.height = 0.35f;
		boneLayout7.radius = 0.15f;
		boneLayout7.shapeType = RagdollShapeType::capsule;

		m_pPhysxSkeleton->AddBone(boneLayout7);

		//Bone 8
		PhysxBoneLayout boneLayout8;
		boneLayout8.name = _T("RightUpperLeg");
		boneLayout8.height = 0.3f;
		boneLayout8.radius = 0.2f;
		boneLayout8.shapeType = RagdollShapeType::capsule;

		m_pPhysxSkeleton->AddBone(boneLayout8);

		//Bone 9
		PhysxBoneLayout boneLayout9;
		boneLayout9.name = _T("RightLowerLeg");
		boneLayout9.height = 0.5f;
		boneLayout9.radius = 0.2f;
		boneLayout9.shapeType = RagdollShapeType::capsule;

		m_pPhysxSkeleton->AddBone(boneLayout9);

		//Bone 10
		PhysxBoneLayout boneLayout10;
		boneLayout10.name = _T("LeftUpperLeg");
		boneLayout10.height = 0.3f;
		boneLayout10.radius = 0.2f;
		boneLayout10.shapeType = RagdollShapeType::capsule;

		m_pPhysxSkeleton->AddBone(boneLayout10);

		//Bone 11
		PhysxBoneLayout boneLayout11;
		boneLayout11.name = _T("LeftLowerLeg");
		boneLayout11.height = 0.5f;
		boneLayout11.radius = 0.2f;
		boneLayout11.shapeType = RagdollShapeType::capsule;

		m_pPhysxSkeleton->AddBone(boneLayout11);

		//---------------------------------------------------------
		//The axisOrientation is in global space.
		//If you want to give it a certain direction you need to add
		//the normalized direction yourself. 
		//Information normalize manually: http://www.fundza.com/vectors/normalize/
		//Eg: Normalize(position2 - position1).

		//Joint 1
		PhysxJointLayout jointLayout;
		jointLayout.jointType = JointType::spherical;
		jointLayout.pBone1 = m_pPhysxSkeleton->GetPhysxBone(boneLayout1);
		jointLayout.pBone2 = m_pPhysxSkeleton->GetPhysxBone(boneLayout2);
		jointLayout.anchorBone = JointBone::PhysxBone2;
		jointLayout.axisOrientation = NxVec3(0,1,0);

		m_pPhysxSkeleton->AddJoint(jointLayout);

		//Joint 2
		PhysxJointLayout jointLayout2;
		jointLayout2.jointType = JointType::spherical;
		jointLayout2.pBone1 = m_pPhysxSkeleton->GetPhysxBone(boneLayout2);
		jointLayout2.pBone2 = m_pPhysxSkeleton->GetPhysxBone(boneLayout3);
		jointLayout2.anchorBone = JointBone::PhysxBone2;
		jointLayout2.axisOrientation = NxVec3(0,1,0);

		m_pPhysxSkeleton->AddJoint(jointLayout2);

		//Joint 3
		PhysxJointLayout jointLayout3;
		jointLayout3.jointType = JointType::spherical;
		jointLayout3.pBone1 = m_pPhysxSkeleton->GetPhysxBone(boneLayout2);
		jointLayout3.pBone2 = m_pPhysxSkeleton->GetPhysxBone(boneLayout4);
		jointLayout3.anchorBone = JointBone::PhysxBone2;
		jointLayout3.axisOrientation = NxVec3(-0.32197f,-0.946653f,0);

		m_pPhysxSkeleton->AddJoint(jointLayout3);

		//Joint 4
		PhysxJointLayout jointLayout4;
		jointLayout4.jointType = JointType::spherical; //rev
		jointLayout4.pBone1 = m_pPhysxSkeleton->GetPhysxBone(boneLayout4);
		jointLayout4.pBone2 = m_pPhysxSkeleton->GetPhysxBone(boneLayout5);
		jointLayout4.anchorBone = JointBone::PhysxBone2;
		jointLayout4.axisOrientation = NxVec3(-0.32197f,-0.946653f,0);

		m_pPhysxSkeleton->AddJoint(jointLayout4);

		//Joint 5
		PhysxJointLayout jointLayout5;
		jointLayout5.jointType = JointType::spherical;
		jointLayout5.pBone1 = m_pPhysxSkeleton->GetPhysxBone(boneLayout2);
		jointLayout5.pBone2 = m_pPhysxSkeleton->GetPhysxBone(boneLayout6);
		jointLayout5.anchorBone = JointBone::PhysxBone2;
		jointLayout5.axisOrientation = NxVec3(0.285960f,-0.9582864f,0);

		m_pPhysxSkeleton->AddJoint(jointLayout5);

		//Joint 6
		PhysxJointLayout jointLayout6;
		jointLayout6.jointType = JointType::spherical; //rev
		jointLayout6.pBone1 = m_pPhysxSkeleton->GetPhysxBone(boneLayout6);
		jointLayout6.pBone2 = m_pPhysxSkeleton->GetPhysxBone(boneLayout7);
		jointLayout6.anchorBone = JointBone::PhysxBone2;
		jointLayout6.axisOrientation = NxVec3(0.285960f,-0.9582864f,0);

		m_pPhysxSkeleton->AddJoint(jointLayout6);

		//Joint 7
		PhysxJointLayout jointLayout7;
		jointLayout7.jointType = JointType::spherical;
		jointLayout7.pBone1 = m_pPhysxSkeleton->GetPhysxBone(boneLayout1);
		jointLayout7.pBone2 = m_pPhysxSkeleton->GetPhysxBone(boneLayout8);
		jointLayout7.anchorBone = JointBone::PhysxBone2;
		jointLayout7.axisOrientation = NxVec3(0,-1,0);

		m_pPhysxSkeleton->AddJoint(jointLayout7);

		//Joint 8
		PhysxJointLayout jointLayout8;
		jointLayout8.jointType = JointType::spherical; //rev
		jointLayout8.pBone1 = m_pPhysxSkeleton->GetPhysxBone(boneLayout8);
		jointLayout8.pBone2 = m_pPhysxSkeleton->GetPhysxBone(boneLayout9);
		jointLayout8.anchorBone = JointBone::PhysxBone2;
		jointLayout8.axisOrientation = NxVec3(0,-1,0);

		m_pPhysxSkeleton->AddJoint(jointLayout8);

		//Joint 9
		PhysxJointLayout jointLayout9;
		jointLayout9.jointType = JointType::spherical;
		jointLayout9.pBone1 = m_pPhysxSkeleton->GetPhysxBone(boneLayout1);
		jointLayout9.pBone2 = m_pPhysxSkeleton->GetPhysxBone(boneLayout10);
		jointLayout9.anchorBone = JointBone::PhysxBone2;
		jointLayout9.axisOrientation = NxVec3(0,-1,0);

		m_pPhysxSkeleton->AddJoint(jointLayout9);

		//Joint 10
		PhysxJointLayout jointLayout10;
		jointLayout10.jointType = JointType::spherical; //rev
		jointLayout10.pBone1 = m_pPhysxSkeleton->GetPhysxBone(boneLayout10);
		jointLayout10.pBone2 = m_pPhysxSkeleton->GetPhysxBone(boneLayout11);
		jointLayout10.anchorBone = JointBone::PhysxBone2;
		jointLayout10.axisOrientation = NxVec3(0,-1,0);

		m_pPhysxSkeleton->AddJoint(jointLayout10);

		//---------------------------------------------------------
		//Build skeleton
		m_pPhysxSkeleton->Initiliaze(m_pMeshFilter);
		m_pPhysxSkeleton->CreateJoints();
	}
}

void PhysicsAnimator::UpdateLeechMode(GameContext& context)
{
	if(m_pPhysxSkeleton != nullptr 
		&& m_currentRagdollState == RagdollState::LeechState)
	{
		//Feed the Animation Data to the PhysxSkeleton if in LeechState
		m_pPhysxSkeleton->FeedBoneTransforms(m_BoneOriginalTransforms);
		//Update the skeleton
		m_pPhysxSkeleton->UpdateLeechMode(context);
	}
}

void PhysicsAnimator::UpdateSeedMode(GameContext& context)
{
	if(m_pPhysxSkeleton != nullptr &&
		m_currentRagdollState == RagdollState::SeedState)
	{
		//Calculate the new bone transform
		m_pPhysxSkeleton->UpdateSeedMode(context);
		//Store them so we can seed them
		m_BonePhysicsTransforms = m_pPhysxSkeleton->SeedBoneTransforms();
	}
}

void PhysicsAnimator::PrepareForLeech()
{
	if(m_pPhysxSkeleton == nullptr)
		return;

	for(auto physxBone : m_pPhysxSkeleton->GetPhysxBones())
	{
		//Set all actors kinematic
		physxBone->RaiseBodyFlag(NX_BF_KINEMATIC);

		//Disable collision detection for all actors
		physxBone->RaiseActorFlag(NX_AF_DISABLE_COLLISION);
	}

	//Delete all joints
	//m_pPhysxSkeleton->ReleaseJoints();
}

void PhysicsAnimator::PrepareForSeed()
{
	if(m_pPhysxSkeleton == nullptr)
		return;

	//Create all proper joints between the PhysxBones
	//m_pPhysxSkeleton->CreateJoints();

	for(auto physxBone : m_pPhysxSkeleton->GetPhysxBones())
	{
		//Set all actor dynamic
		physxBone->ClearBodyFlag(NX_BF_KINEMATIC);

		//Enable collision detection for all actor
		physxBone->ClearActorFlag(NX_AF_DISABLE_COLLISION);
	}
}

void PhysicsAnimator::SetCurrentState(RagdollState state)
{
	//store the state if it is not allready the current state
	//else return so we won't prepare anything again
	if(m_currentRagdollState != state)
		m_currentRagdollState = state;
	else
		return;

	//prepare the skeleton based on the new state
	if(m_currentRagdollState == RagdollState::LeechState)
		PrepareForLeech();
	else if(m_currentRagdollState == RagdollState::SeedState)
		PrepareForSeed();
}

void PhysicsAnimator::SetWorldTransform(const D3DXMATRIX& worldTransform)
{
	//Store the worldtransform
	m_matWorldTransform = worldTransform;

	//Pass it to the skeleton
	if(m_pPhysxSkeleton)
		m_pPhysxSkeleton->SetWorldTransform(m_matWorldTransform);
}

PhysxSkeleton* PhysicsAnimator::GetSkeleton() const
{
	if(m_pPhysxSkeleton != nullptr)
		return m_pPhysxSkeleton;

	return nullptr;
}