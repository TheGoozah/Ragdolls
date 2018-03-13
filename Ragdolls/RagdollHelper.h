#ifndef RAGDOLLHELPER_H_INCLUDED_
#define RAGDOLLHELPER_H_INCLUDED_
//--------------------------------------------------------------------------------------
// Structs and Enums used to for the ragdolls in OverlordEngine
// Created by Matthieu Delaere
//--------------------------------------------------------------------------------------
#pragma once
#include "../../../OverlordEngine/Helpers/stdafx.h"
class PhysxBone;

enum RagdollShapeType
{
	capsule,
	sphere
};

enum RagdollState
{
	StateError = -1,
	LeechState,
	SeedState
};

enum JointType
{
	spherical,
	revolute
};

enum JointBone
{
	PhysxBone1,
	PhysxBone2
};

struct PhysxBoneLayout
{
	//Constructor to make sure all variables are initialized
	PhysxBoneLayout(void):
		name(_T("")), shapeType(RagdollShapeType::capsule),
		height(5.0f), radius(1.0f)
	{}

	tstring name; //name of bone we map to
	RagdollShapeType shapeType; //the type of shape we will use for the actor
	float height; //the height of the shape if it has any
	float radius; //the radius for the shape
};

struct PhysxJointLayout
{
	//Constructor to make sure all variables are initialized
	PhysxJointLayout(void):
		jointType(JointType::spherical), pBone1(nullptr),
		pBone2(nullptr), anchorBone(JointBone::PhysxBone1),
		axisOrientation(NxVec3(1,0,0))
	{}

	JointType jointType; //type of joint
	PhysxBone* pBone1; //pointer to physxBone 1
	PhysxBone* pBone2; //pointer to physxBone 2
	JointBone anchorBone; //position of the anchor (global)
	NxVec3 axisOrientation; //normalized vector indicating the axis along we create our joint
};
#endif