//--------------------------------------------------------------------------------------
// Enemy - Base
// Created by Matthieu Delaere
//--------------------------------------------------------------------------------------
#include "Enemy.h"
#include "../../../OverlordEngine/Scenegraph/GameScene.h"
#include "../../../OverlordEngine/Managers/PhysicsManager.h"
#include "../../../OverlordEngine/Diagnostics/Logger.h"
#include "../../../OverlordEngine/Graphics/GraphicsDevice.h"

#include "../../SZS_Materials/SkinnedShadowGenerationMaterial.h"
#include "../../SZS_Materials/SkinnedMaterial.h"
#include "../Managers/EnemyManager.h"
#include "../Ragdolls/PhysicsAnimator.h"
#include "../Targets/Target.h"

#include "../ErrorHandling/ErrorHandles.h"

Enemy::Enemy(EnemyManager* pOwnerEnemyManager, float destroyInterval):
	m_pOwnerEnemyManager(pOwnerEnemyManager),
	m_pModelComponent(nullptr),
	m_pControllerComponent(nullptr),
	m_pSkinnedShadowGenerationMaterial(nullptr), m_pSkinnedMaterial(nullptr),
	m_fDestroyInterval(destroyInterval),
	m_fPersonalDestroyTimer(destroyInterval),
	m_eCurrentState(GameHelper::EnemyState::Walking),
	m_eCurrentInteractState(GameHelper::EnemyInteractState::Released),
	m_pTarget(nullptr),
	m_fWidth(1.3f), m_fHeight(3.5f), m_fHeightOffset(0.0f),
	m_fGravity(9.81f),
	m_fGravityAccelerationTime(0.3f), m_fGravityAcceleration(0.0f),
	m_fGravityVelocity(0.0f), m_fTerminalVelocity(0.5f),
	m_fWalkSpeed(7.0f), m_fMaximumWalkingSpeed(4.0f),
	m_Velocity(D3DXVECTOR3(0,0,0)),
	m_fCurrentRecoverTime(0.0f), m_fTotalRecoverTime(4.0f),
	m_bFlaggedToRemoveUnderY(false),
	m_bIsPickable(true), m_fMaximumTimeUnpickable(5.0f), m_fCurrentTimeUnpickable(0.0f),
	m_bIsShowcase(true)
{
	 m_fGravityAcceleration = m_fGravity/m_fGravityAccelerationTime;
	 m_fHeightOffset = -(m_fHeight/2 + 0.5f);
}

Enemy::~Enemy(void)
{
	SafeDelete(m_pSkinnedMaterial);
	SafeDelete(m_pSkinnedShadowGenerationMaterial);
}

void Enemy::Initialize()
{
	//-------------------------------------
	//Setup our Enemy
	//-------------------------------------
	//Materials
	D3DXVECTOR3 cameraPos = D3DXVECTOR3(0,60,-35); 
	D3DXVECTOR3 targetPos = D3DXVECTOR3(0,0,0);
	D3DXVECTOR3 direction = targetPos - cameraPos;
	D3DXVec3Normalize(&direction, &direction);

	m_pSkinnedShadowGenerationMaterial = new SkinnedShadowGenerationMaterial(direction, cameraPos);
	m_pSkinnedShadowGenerationMaterial->SetTexture(_T("./SZS_Resources/Textures/zombieDiffuse.png"));

	m_pSkinnedMaterial = new SkinnedMaterial(_T("./SZS_Resources/Textures/zombieDiffuse.png"));

	//Model + setup for ragdoll
	m_pModelComponent = new ModelComponent(_T("./SZS_Resources/Models/baseZombieAnim.ovm"), 
			m_pOwnerEnemyManager->GetOwnerScene()->GetPhysicsScene());
	m_pModelComponent->SetShadowMaterial(m_pSkinnedShadowGenerationMaterial);
	m_pModelComponent->SetMaterial(m_pSkinnedMaterial);
	m_pModelComponent->SetPhysxGroup(PhysicsGroup::Layer2);
	m_pModelComponent->SetCullingEnabled(false);
	this->AddComponent(m_pModelComponent);

	//CharacterController 
	m_pControllerComponent = new ControllerComponent(m_fWidth, m_fHeight, m_fHeightOffset, 
		false, PhysicsGroup::Layer5, _T("Zombie"));
	this->AddComponent(m_pControllerComponent);

	//Base Init
	GameObject::Initialize();
}

void Enemy::Update(GameContext& context)
{
	//Check if all components exist first
	if(m_pControllerComponent == nullptr)
		return;
	if(m_pModelComponent == nullptr)
		return;
	if(!m_bIsShowcase && m_pTarget == nullptr)
		return;


	//---------------------------------------------
	//Check our interactive states (if not dead)
	//---------------------------------------------
	//If linked enemy must paralyze
	if(m_eCurrentInteractState == GameHelper::EnemyInteractState::Linked
		&& m_eCurrentState != GameHelper::EnemyState::Dead)
	{
		m_eCurrentState = GameHelper::EnemyState::Paralyzed;
	}
	
	//---------------------------------------------
	//Check our ai states
	//---------------------------------------------
	//Disable the controller untill we support a global actor class that can cast void*
	//to a class where we can check it's type. See PhysxContactReport.h for more information
	//about the issue. Drawback at this moment is that our ragdolls don't have collision
	//with eachother.
	//m_pControllerComponent->DisableController();
	//Also set the correct ragdoll state for each AI state. Will be checked internal if it has changes or not.
	//If it has, the physicsAnimator will do the necessary steps for the ragdoll skeleton!
	if(m_eCurrentState == GameHelper::EnemyState::Walking)
	{
		m_pControllerComponent->ActivateController();
		//Set State
		SetRagdollState(RagdollState::LeechState);
		//Set Animation Clip
		m_pModelComponent->SetAnimationClip(_T("Walk"));
		//Move the enemy
		MoveEnemyController(context);
	}
	else if(m_eCurrentState == GameHelper::EnemyState::Paralyzed)
	{
		//Disable controller first!!!
		m_pControllerComponent->DisableController();
		//Set the ragdoll state if needed
		SetRagdollState(RagdollState::SeedState);

		//Position controller
		D3DXVECTOR3 position = this->GetPositionRootBone();
		position.y = 1.0f;
		position.z = 0;
		m_pControllerComponent->Translate(position);

		//If enemy is paralyzed and not linked check if he hasn't moved
		//for x amount of seconds. If not let him recover
		if(m_eCurrentInteractState != GameHelper::EnemyInteractState::Linked)
		{
			//Countdown
			if(IsEnemyMoving() == false)
				m_fCurrentRecoverTime += context.GameTime.ElapsedSeconds();
			else
				m_fCurrentRecoverTime = 0.0f;

			//If needs to recover, recover
			if(m_fCurrentRecoverTime >= m_fTotalRecoverTime)
			{
				m_eCurrentState = GameHelper::EnemyState::Recovering;
				m_fCurrentRecoverTime = 0.0f;
			}
		}
	}
	else if(m_eCurrentState == GameHelper::EnemyState::Dead)
	{
		if (m_bIsShowcase)
		{
			m_eCurrentState = GameHelper::EnemyState::Paralyzed;
			return;
		}

		//Disable controller first!!!
		m_pControllerComponent->DisableController();
		//Set the ragdoll state if needed
		SetRagdollState(RagdollState::SeedState);

		//Position controller
		D3DXVECTOR3 position = this->GetPositionRootBone();
		position.y = 1.0f;
		position.z = 0;
		m_pControllerComponent->Translate(position);
	}
	else if(m_eCurrentState == GameHelper::EnemyState::Recovering)
	{
		//Enable controller
		m_pControllerComponent->ActivateController();
		//Set the ragdoll state if needed
		SetRagdollState(RagdollState::LeechState);
		//Set Animation Clip
		m_pModelComponent->SetAnimationClip(_T("Walk"));
		//Reset velocity
		m_Velocity = D3DXVECTOR3(0,0,0);
		//Reset position
		if(m_bIsShowcase)
			m_pControllerComponent->Translate(D3DXVECTOR3(0, 0, 0));
		//Change state
		m_eCurrentState = GameHelper::EnemyState::Walking;
	}
	else if(m_eCurrentState == GameHelper::EnemyState::Attacking)
	{
		//Walking mode for showcasing
		if (m_bIsShowcase)
		{
			m_eCurrentState = GameHelper::EnemyState::Walking;
			return;
		}
		//Set the ragdoll state if needed
		SetRagdollState(RagdollState::LeechState);
		//Set Animation Clip
		m_pModelComponent->SetAnimationClip(_T("Attack"));
		//do damage to target
		if(m_pTarget)
		{
			float currentHealth = m_pTarget->GetTargetHealth();
			float damageFactor = 5.0f;
			float newHealth = currentHealth - (damageFactor * context.GameTime.ElapsedSeconds());
			if(newHealth > 0)
				m_pTarget->SetTargetHealth(newHealth);
			else
				m_pTarget->SetTargetHealth(0.0f);
		}
	}

	//---------------------------------------------
	//Kill Y
	//---------------------------------------------
	//When for some reason an enemy keeps falling (glitched through wall, dropped by player, etc)
	//we kill it if it goes under a certain value. (Flag itself to the manager AND remember it)
	float lowestYValue = -20.0f;
	float currentYValue = this->GetComponent<TransformComponent>()->GetWorldPosition().y;

	if(currentYValue < lowestYValue && m_bFlaggedToRemoveUnderY == false)
	{
		m_pOwnerEnemyManager->ForceEnemyRemoval(this);
		m_bFlaggedToRemoveUnderY = true;
	}

	//---------------------------------------------
	//Unpickable
	//---------------------------------------------
	if(m_bIsPickable == false)
	{
		//change color enemy
		m_pSkinnedMaterial->SetColor(D3DXCOLOR(1,0,0,1));

		//time it
		m_fCurrentTimeUnpickable += context.GameTime.ElapsedSeconds();

		//if it has reached the maximum set it pickable again
		if(m_fCurrentTimeUnpickable >= m_fMaximumTimeUnpickable)
		{
			m_fCurrentTimeUnpickable = 0.0f;
			m_bIsPickable = true;
		}
	}
	else
	{
		//change color enemy
		m_pSkinnedMaterial->SetColor(D3DXCOLOR(1,1,1,1));

		//reset timer to be sure it restarts if programmer hard switches the state
		m_fCurrentTimeUnpickable = 0.0f;
	}

	//Base Update
	GameObject::Update(context);
}

void Enemy::MoveEnemyController(GameContext& context)
{
	auto currentPosition = m_pControllerComponent->GetTranslation();
	D3DXVECTOR3 targetPosition = D3DXVECTOR3(0,0,0);
	if(m_pTarget)
		targetPosition = m_pTarget->GetTargetPosition();
	D3DXVECTOR3 toTargetDirection = targetPosition - currentPosition;
	D3DXVECTOR3 toTargetDirectionNormalized;
	D3DXVec3Normalize(&toTargetDirectionNormalized, &toTargetDirection);

	//**********************
	//Model Rotation to Target
	D3DXMATRIX lookToRotationMatrix;
	D3DXMatrixLookAtLH(&lookToRotationMatrix, &currentPosition, &(toTargetDirectionNormalized + currentPosition),
		&D3DXVECTOR3(0,1.0f,0));
	D3DXQUATERNION lookToRotationQuaternion;
	D3DXQuaternionRotationMatrix(&lookToRotationQuaternion, &lookToRotationMatrix);

	lookToRotationQuaternion.x = 0.0f; //Discard x and z rotation
	lookToRotationQuaternion.z = 0.0f;
	this->GetComponent<TransformComponent>()->Rotate(lookToRotationQuaternion);

	//**********************
	//Translation to Target
	//If distance isn't smaller than x
	float distance = D3DXVec3Length(&toTargetDirection);
	if(distance > m_fWidth)
	{
		m_Velocity += toTargetDirectionNormalized * m_fWalkSpeed * context.GameTime.ElapsedSeconds();
		if(m_Velocity.x > m_fMaximumWalkingSpeed)
			m_Velocity.x = m_fMaximumWalkingSpeed;
		if(m_Velocity.x < -m_fMaximumWalkingSpeed)
			m_Velocity.x = -m_fMaximumWalkingSpeed;
	}
	else
	{
		//if distance is smaller, go to attack state
		m_eCurrentState = GameHelper::EnemyState::Attacking;
	}

	if (m_bIsShowcase)
	{
		m_Velocity = D3DXVECTOR3(0, 0, 0);
		this->GetComponent<TransformComponent>()->Rotate(D3DXQUATERNION(0, 0, 0, 1));
	}

	//**********************
	//Gravity
	if(!HasContactWithFloor(currentPosition))
	{
		m_fGravityVelocity -= m_fGravityAcceleration * context.GameTime.ElapsedSeconds();
		if(m_fGravityVelocity < -m_fTerminalVelocity)
			m_fGravityVelocity = -m_fTerminalVelocity;
	}
	else
	{
		m_fGravityVelocity = 0;
		m_Velocity.y = 0;
	}

	m_Velocity.y += m_fGravityVelocity;

	//MOVE CONTROLLER
	m_pControllerComponent->Move(m_Velocity * context.GameTime.ElapsedSeconds());
}

bool Enemy::HasContactWithFloor(D3DXVECTOR3 position) const
{
	NxVec3 start(position.x, position.y, position.z);
	NxVec3 direction(0,-1.0f,0);
	NxRay ray(start, direction);
	NxRaycastHit hit;
	float distance = 0.8f;

	auto shape = PhysicsManager::GetInstance()->GetClosestShape(GetGameScene()->GetPhysicsScene(),
		ray, hit, PhysicsGroup::Layer1, NX_STATIC_SHAPES, distance);
	return shape != nullptr;
}

bool Enemy::IsEnemyMoving() const
{
	if(m_pModelComponent == nullptr)
		return false;

	//Check if any of the actors is moving - RAGDOLL COMPONENT ACTORS
	float comparisonValue = 4.0f;

	vector<NxActor*> vActorsSkeleton;
	vActorsSkeleton = m_pModelComponent->GetPhysxSkeleton()->GetBoneActors();

	for(auto actor : vActorsSkeleton)
	{
		float x, y, z;
		x = actor->getLinearVelocity().x;
		y = actor->getLinearVelocity().y;
		z = actor->getLinearVelocity().z;

		if(x > comparisonValue || y > comparisonValue || z > comparisonValue)
			return true;
	}
	
	return false;
}

void Enemy::SetContactReportThreshold(float value)
{
	if(m_pModelComponent == nullptr)
		return;

	vector<NxActor*> vActorsSkeleton;
	vActorsSkeleton = m_pModelComponent->GetPhysxSkeleton()->GetBoneActors();

	for(auto actor : vActorsSkeleton)
	{
		actor->setContactReportThreshold(value);
	}
}

void Enemy::SetContactReportFlags(NxU32 flags)
{
	if(m_pModelComponent == nullptr)
		return;

	vector<NxActor*> vActorsSkeleton;
	vActorsSkeleton = m_pModelComponent->GetPhysxSkeleton()->GetBoneActors();

	for(auto actor : vActorsSkeleton)
	{
		actor->setContactReportFlags(flags);
	}
}

vector<NxActor*> Enemy::GetRagdollActors() const
{
	vector<NxActor*> vRagdollActors;

	if(m_pModelComponent != nullptr)
	{
		vRagdollActors = m_pModelComponent->GetPhysxSkeleton()->GetBoneActors();
	}

	return vRagdollActors;
}

void Enemy::SetRagdollState(RagdollState state)
{
	if(m_pModelComponent != nullptr)
	{
		PhysicsAnimator* physxAnimator = m_pModelComponent->GetPhysxAnimator();
		if(physxAnimator != nullptr)
			physxAnimator->SetCurrentState(state);
	}
}

const RagdollState Enemy::GetRagdollState() const
{
	RagdollState state = RagdollState::StateError;

	if(m_pModelComponent != nullptr)
	{
		PhysicsAnimator* physxAnimator = m_pModelComponent->GetPhysxAnimator();
		if(physxAnimator != nullptr)
			state = physxAnimator->GetCurrentState();
	}

	return state;
}

D3DXVECTOR3 Enemy::GetPositionRootBone() const
{
	PhysxSkeleton* pSkeleton = nullptr;
	NxActor* pRootBoneActor = nullptr;
	D3DXVECTOR3 rootBonePosition = D3DXVECTOR3(0,0,0);

	if(m_pModelComponent != nullptr)
		pSkeleton = m_pModelComponent->GetPhysxSkeleton();

	if(pSkeleton != nullptr)
		pRootBoneActor = pSkeleton->GetRootBoneActor();

	if(pRootBoneActor != nullptr)
	{
		rootBonePosition.x = pRootBoneActor->getGlobalPosition().x;
		rootBonePosition.y = pRootBoneActor->getGlobalPosition().y;
		rootBonePosition.z = pRootBoneActor->getGlobalPosition().z;
	}

	return rootBonePosition;
}

void Enemy::SetPositionEnemy(const D3DXVECTOR3& position)
{
	if(m_pControllerComponent != nullptr)
		m_pControllerComponent->Translate(position);
}

D3DXVECTOR3 Enemy::GetPositionEnemy() const
{
	D3DXVECTOR3 position = D3DXVECTOR3(0,0,0);

	if(m_pControllerComponent != nullptr)
		position = m_pControllerComponent->GetTranslation();

	return position;
}