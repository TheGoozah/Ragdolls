#ifndef SERIALIZER_H_INCLUDED_
#define SERIALIZER_H_INCLUDED_
//--------------------------------------------------------------------------------------
// The Serializer is responsable for saving and loading the states of the the current
// game. It saves the state, but also the active wave and the active enemies.
// Created by Matthieu Delaere
//--------------------------------------------------------------------------------------
#pragma once
#include "../../../OverlordEngine/Helpers/stdafx.h"
#include "../../../OverlordEngine/Helpers/D3DUtil.h"
#include "../../../OverlordEngine/Helpers/PhysicsHelper.h"
#include "../Ragdolls/RagdollHelper.h"
#include "../GameHelper.h"

#include "../../SZS_Tools/PugiXML/pugixml.hpp"

#include <vector>

class GameDirector;
class Enemy;

struct RollBackEnemy final
{
	RollBackEnemy(void):enemyState(GameHelper::EnemyState::Walking), ragdollState(RagdollState::LeechState),
		amountOfRagdollActors(0), positionController(D3DXVECTOR3(0,0,0))
	{}

	~RollBackEnemy(void)
	{
		ragdollActorTransforms.clear();
		ragdollActorLinVel.clear();
	}

	GameHelper::EnemyState enemyState;
	RagdollState ragdollState;
	UINT amountOfRagdollActors;
	D3DXVECTOR3 positionController;
	vector<NxMat34> ragdollActorTransforms;
	vector<NxVec3> ragdollActorLinVel;
};

struct RollBackWave final
{
	RollBackWave(void):spawnPosition(D3DXVECTOR3(0,0,0)), enemiesLeftToSpawn(0),
		spawnInterval(0.0f), currentInterval(0.0f)
	{}

	D3DXVECTOR3 spawnPosition;
	UINT enemiesLeftToSpawn;
	float spawnInterval;
	float currentInterval;
};

struct RollBackScore final
{
	RollBackScore(void):position(D3DXVECTOR3(0,0,0)), currentLifeTime(0.0f)
	{}

	D3DXVECTOR3 position;
	float currentLifeTime;
};

struct RollBackForceField final
{
	RollBackForceField(void):position(D3DXVECTOR3(0,0,0)), scale(D3DXVECTOR3(1,1,1)),
		currentLifeTime(0.0f), groupID(PhysicsGroup::Layer7)
	{}

	D3DXVECTOR3 position;
	D3DXVECTOR3 scale;
	float currentLifeTime;
	PhysicsGroup groupID;
};

class RollBackContainer final
{
public:
	RollBackContainer(void):
		m_score(0), m_targetHealth(0), m_waveLevel(0), m_amountOfSmashAllPower(0), m_bSpawningOnHold(false)
	{};
	~RollBackContainer(void)	
	{
		m_newEnemies.clear();
		m_newWaves.clear();
		m_newScoreObjects.clear();
		m_newForceFieldObjects.clear();
	};

	void SetScore(UINT score){m_score = score;};
	void SetTargetHealth(float value){m_targetHealth = value;};
	void SetWaveLevel(UINT value){m_waveLevel = value;};
	void SetAmountSmashAllPowers(UINT value){m_amountOfSmashAllPower = value;};
	void SetSpawningOnHold(bool state){m_bSpawningOnHold = state;};
	void AddRollBackWave(const RollBackWave& rbWave){m_newWaves.push_back(rbWave);};
	void AddRollBackEnemy(const RollBackEnemy& rbEnemy){m_newEnemies.push_back(rbEnemy);};
	void AddRollBackScoreObject(const RollBackScore& rbScore){m_newScoreObjects.push_back(rbScore);};
	void AddForceFieldObject(const RollBackForceField& rbForceField){m_newForceFieldObjects.push_back(rbForceField);};

	UINT GetScore() const {return m_score;};
	float GetTargetHealth() const {return m_targetHealth;};
	UINT GetWaveLevel() const {return m_waveLevel;};
	UINT GetAmountSmashAllPowers() const {return m_amountOfSmashAllPower;};
	bool IsSpawningOnHold() const {return m_bSpawningOnHold;};
	vector<RollBackWave> GetWaves() const {return m_newWaves;};
	vector<RollBackEnemy> GetEnemies() const {return m_newEnemies;};
	vector<RollBackScore> GetScoreObjects() const {return m_newScoreObjects;};
	vector<RollBackForceField> GetForceFieldObjects() const {return m_newForceFieldObjects;};

private:
	//GameInfo
	UINT m_score;
	float m_targetHealth;
	UINT m_waveLevel;
	UINT m_amountOfSmashAllPower;
	bool m_bSpawningOnHold;

	//Waves
	vector<RollBackWave> m_newWaves;

	//Active Enemies
	vector<RollBackEnemy> m_newEnemies;

	//Active Score Objects
	vector<RollBackScore> m_newScoreObjects;

	//Active Force Fields
	vector<RollBackForceField> m_newForceFieldObjects;

	// -------------------------
	// Disabling default copy constructor and default 
	// assignment operator.
	// -------------------------
	RollBackContainer(const RollBackContainer& yRef);									
	RollBackContainer& operator=(const RollBackContainer& yRef);
};

class Serializer final
{
public:
	Serializer(GameDirector* pOwnerDirector);
	~Serializer(void);

	//METHODS
	bool SaveGame(const tstring& pathSaveGame);
	bool LoadGame(const tstring& pathSaveGame);
	bool SaveHighScore(const tstring& pathHighScore);
	bool LoadHighScore(const tstring& pathHighScore);

private:
	//DATAMEMBERS
	GameDirector* m_pOwnerGameDirector;

	//METHODS
	bool SerializeGameInfo(tstringstream& ss, int depth = 0);
	bool SerializeActiveWaves(tstringstream& ss, int depth = 0);
	bool SerializeActiveEnemies(tstringstream& ss, int depth = 0);
	bool SerializeActiveScoreObjects(tstringstream& ss, int depth = 0);
	bool SerializeActiveForceFields(tstringstream& ss, int depth = 0);

	bool LoadGameInfo(pugi::xml_node root, RollBackContainer& rollbackContainer);
	bool LoadWaves(pugi::xml_node root, RollBackContainer& rollbackContainer);
	bool LoadEnemies(pugi::xml_node root, RollBackContainer& rollbackContainer);
	bool LoadScoreObjects(pugi::xml_node root, RollBackContainer& rollbackContainer);
	bool LoadForceFieldObjects(pugi::xml_node root, RollBackContainer& rollbackContainer);

	bool LoadSaveGameInMemory(const RollBackContainer& rollbackContainer);

	bool LoadEnemyType0(pugi::xml_node physXStatesEnemy, RollBackContainer& rollbackContainer, RollBackEnemy& rbEnemy, 
		UINT amountOfRagdollActors = 0);

	// -------------------------
	// Disabling default copy constructor and default 
	// assignment operator.
	// -------------------------
	Serializer(const Serializer& yRef);									
	Serializer& operator=(const Serializer& yRef);
};
#endif