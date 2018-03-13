#ifndef FORCEFIELD_H_INCLUDED_
#define FORCEFIELD_H_INCLUDED_
//--------------------------------------------------------------------------------------
// ForceFieldObject - Used to create a force field and maintain it
// Created by Matthieu Delaere
//--------------------------------------------------------------------------------------
#pragma once
#include "../../../OverlordEngine/Helpers/stdafx.h"
#include "../../../OverlordEngine/Helpers/D3DUtil.h"
#include "../../../OverlordEngine/Helpers/PhysicsHelper.h"
#include "../../../OverlordEngine/Helpers/GeneralStructs.h"

class GameDirector;

class ForceFieldObject final
{
public:
	ForceFieldObject(GameDirector* pOwnerDirector, PhysicsGroup groupID, const NxVec3& sizeShape = NxVec3(1,1,1),
		const NxVec3& position = NxVec3(0,0,0));
	~ForceFieldObject(void);

	//METHODS
	NxForceField* CreateForceField();
	
	void Update(GameContext& context);

	void SetMaximumLifeTime(float maxLife){m_fMaximumLifeTime = maxLife;};
	void SetCurrentLifeTime(float currentLife){m_fCurrentLifeTime = currentLife;};
	void SetForce(const NxVec3& force){m_Force = force;};

	bool ReachedEndLifeTime()const;
	float GetCurrentLifeTime() const {return m_fCurrentLifeTime;};
	NxVec3 GetCurrentPosition() const {return m_Position;};
	NxVec3 GetCurrentScale() const {return m_SizeShape;};

private:
	//DATAMEMBERS
	GameDirector* m_pOwnerDirector;
	PhysicsGroup m_groupID;
	NxVec3 m_SizeShape;
	NxVec3 m_Position;

	NxForceFieldLinearKernel* m_pLinearKernel;
	NxForceField* m_pForceField;

	float m_fCurrentLifeTime;
	float m_fMaximumLifeTime;

	NxVec3 m_Force;

	//METHODS
	void ReleaseResources();

	// -------------------------
	// Disabling default copy constructor and default 
	// assignment operator.
	// -------------------------
	ForceFieldObject(const ForceFieldObject& yRef);									
	ForceFieldObject& operator=(const ForceFieldObject& yRef);
};
#endif