#ifndef ENEMY_H_INCLUDED_
#define ENEMY_H_INCLUDED_
//--------------------------------------------------------------------------------------
// Enemy - Base
// Created by Matthieu Delaere
//--------------------------------------------------------------------------------------
#pragma once
#include "Scenegraph/GameObject.h"
#include "../../../OverlordEngine/Helpers/stdafx.h"
#include "../../../OverlordEngine/Helpers/D3DUtil.h"
#include "../../../OverlordEngine/OverlordComponents.h"

#include "../GameHelper.h"
#include "../Ragdolls/RagdollHelper.h"

class SkinnedShadowGenerationMaterial;
class SkinnedMaterial;
class EnemyManager;
class Target;

class Enemy final :public GameObject
{
public:
	Enemy(EnemyManager* pOwnerEnemyManager, float destroyInterval = 1.0f);
	virtual ~Enemy(void);

	//METHODS
	virtual void Initialize();
	virtual void Update(GameContext& context);

	//Death timers
	void ResetDeathTimer(){m_fPersonalDestroyTimer = m_fDestroyInterval;};
	float GetDeathTimerValue() const {return m_fPersonalDestroyTimer;};
	void SetDeathTimerValue(float value){m_fPersonalDestroyTimer = value;};

	//Enemy States
	void SetEnemyState(GameHelper::EnemyState state){m_eCurrentState = state;};
	GameHelper::EnemyState GetEnemyState() const {return m_eCurrentState;};
	void SetEnemyInteractState(GameHelper::EnemyInteractState state){m_eCurrentInteractState = state;};
	GameHelper::EnemyInteractState GetEnemyInteractState() const {return m_eCurrentInteractState;};

	//AI Information
	void SetWalkingSpeed(float speed){m_fWalkSpeed = speed;};
	void SetTarget(Target* target ){m_pTarget = target;};

	//Sets an enemy on a certain spot (charactercontroller offcourse)
	void SetPositionEnemy(const D3DXVECTOR3& position);
	//Gets the position of our enemy (charactercontroller)
	D3DXVECTOR3 GetPositionEnemy() const;

	//Setup ContactReport trigger
	void SetContactReportThreshold(float value);
	void SetContactReportFlags(NxU32 flags);

	//Ragdoll Actors
	vector<NxActor*> GetRagdollActors() const;
	void SetRagdollActors();

	//Ragdoll States
	void SetRagdollState(RagdollState state);
	const RagdollState GetRagdollState() const;

	//Checking if Enemy Ragdoll Actors are moving
	bool IsEnemyMoving() const;

	//Checking if Enemy is pickable + set the state
	bool IsEnemyPickable() const { return m_bIsPickable;};
	void SetEnemyPickable(bool state){m_bIsPickable = state;};

private:
	//DATAMEMBERS
	ModelComponent* m_pModelComponent; //Pointer to ModelComponent
	ControllerComponent* m_pControllerComponent; //Pointer to the character controller

	SkinnedMaterial* m_pSkinnedMaterial; //Pointer to the material used by the ModelComponent
	SkinnedShadowGenerationMaterial* m_pSkinnedShadowGenerationMaterial; //Generate shadows that gets projected on other models.
	//Not on the enemy itself (buggy)
	EnemyManager* m_pOwnerEnemyManager; //Pointer to the EnemyManager owning this enemy

	float m_fPersonalDestroyTimer; //Personal timer used when flagged for deletion.
	float m_fDestroyInterval; //The initial value of the DeathTimer before substraction.

	GameHelper::EnemyState m_eCurrentState; //Holding the state the enemy is in.
	GameHelper::EnemyInteractState m_eCurrentInteractState; //Holding the state how the enemy is used on the interaction (input)

	Target* m_pTarget; //The target

	float m_fWidth, m_fHeight, m_fHeightOffset; //Capsule information
	float m_fGravity; //Gravity value
	float m_fGravityVelocity, //Used to calculate the gravity displacement
		m_fGravityAccelerationTime,
		m_fGravityAcceleration;
	float m_fTerminalVelocity; //Limit gravity acceleration
	float m_fWalkSpeed;
	float m_fMaximumWalkingSpeed;

	float m_fCurrentRecoverTime; //Time enemy is paralyzed and not moving
	float m_fTotalRecoverTime; //Time enemy need to be paralyzed before it recovers

	D3DXVECTOR3 m_Velocity; //Total velocity of our controller
	bool m_bFlaggedToRemoveUnderY; //If we are flagged for deletion because of under certain Y value this bool yields true

	bool m_bIsPickable; //State if enemy is pickable or not
	float m_fMaximumTimeUnpickable; //The time enemy is not pickable
	float m_fCurrentTimeUnpickable; //The current time the enemy is not pickable

	bool m_bIsShowcase; //Always puts the enemy Walking State

	//METHODS
	//Moves our enemy to a certain target (rotation model + hold with reaches target)
	void MoveEnemyController(GameContext& context);
	//Checks if controller has contact with floor (PhysxLayer 1)
	bool HasContactWithFloor(D3DXVECTOR3 position) const;
	//Get position rootbone
	D3DXVECTOR3 GetPositionRootBone() const;

	// -------------------------
	// Disabling default copy constructor and default 
	// assignment operator.
	// -------------------------
	Enemy(const Enemy& yRef);									
	Enemy& operator=(const Enemy& yRef);
};
#endif