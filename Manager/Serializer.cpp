//--------------------------------------------------------------------------------------
// The Serializer is responsable for saving and loading the states of the the current
// game. It saves the state, but also the active wave and the active enemies.
// Created by Matthieu Delaere
//--------------------------------------------------------------------------------------
#include "Serializer.h"
#include "../../../OverlordEngine/Scenegraph/GameScene.h"
#include "../../../OverlordEngine/Diagnostics/Logger.h"

#include "GameDirector.h"
#include "EnemyManager.h"
#include "../Enemies/EnemyWave.h"
#include "../Enemies/Enemy.h"
#include "../Scoring/ScoreManager.h"
#include "../Scoring/ScoreObject.h"
#include "ForceFieldManager.h"
#include "../ForceField/ForceFieldObject.h"
#include "../Ragdolls/RagdollHelper.h"
#include "../Targets/Target.h"
#include "../../../OverlordEngine/Managers/PhysicsManager.h"


Serializer::Serializer(GameDirector* pOwnerDirector):
	m_pOwnerGameDirector(pOwnerDirector)
{
}

Serializer::~Serializer(void)
{
}

bool Serializer::SaveGame(const tstring& pathSaveGame)
{
	if(m_pOwnerGameDirector)
	{
		//Before we start writing stuff retrieve handle to file and it's attributes
		DWORD dwAttr;
		WIN32_FIND_DATA FindFileData;
		HANDLE hFind;
		hFind = FindFirstFile(pathSaveGame.c_str(), &FindFileData);
		dwAttr = GetFileAttributes(pathSaveGame.c_str());

		//if the file exists and it's attributes
		//if the file is read-only we make it writable
		if((hFind != INVALID_HANDLE_VALUE) && (dwAttr & FILE_ATTRIBUTE_READONLY))
		{
			SetFileAttributes(pathSaveGame.c_str(), dwAttr &= ~FILE_ATTRIBUTE_READONLY);
		}

		//Write header of the XML
		tstringstream ss;
		ss << _T("<?xml version=\"1.0\"?>\n");

		//Save all the data to the XML file
		ss << _T("<SuperZombieSmash>\n"); //Add global tag

		//-----------------------------------------------
		// GAME INFO (Score + Health + Waves)
		//-----------------------------------------------
		ss << _T("\t");
		ss << _T("<GameInfo>\n");

		//Serialize information GameInfo
		if(!SerializeGameInfo(ss, 2))
			return false;

		ss << _T("\t");
		ss << _T("</GameInfo>\n");

		//-----------------------------------------------
		// SCORE OBJECT INFO
		//-----------------------------------------------
		ss << _T("\t");
		ss << _T("<ActiveScoreObjects>\n");

		if(!SerializeActiveScoreObjects(ss, 2))
			return false;

		ss << _T("\t");
		ss << _T("</ActiveScoreObjects>\n");

		//-----------------------------------------------
		// FORCE FIELD OBJECT INFO
		//-----------------------------------------------
		ss << _T("\t");
		ss << _T("<ActiveForceFieldObjects>\n");

		if(!SerializeActiveForceFields(ss, 2))
			return false;

		ss << _T("\t");
		ss << _T("</ActiveForceFieldObjects>\n");

		//-----------------------------------------------
		// WAVES INFO
		//-----------------------------------------------
		ss << _T("\t");
		ss << _T("<ActiveWaves>\n");

		if(!SerializeActiveWaves(ss, 2))
			return false;

		ss << _T("\t");
		ss << _T("</ActiveWaves>\n");

		//-----------------------------------------------
		// INDIVIDUAL ENEMY INFO
		//-----------------------------------------------
		ss << _T("\t");
		ss << _T("<ActiveEnemies>\n");

		if(!SerializeActiveEnemies(ss, 2))
			return false;

		ss << _T("\t");
		ss << _T("</ActiveEnemies>\n");
		//-----------------------------------------------
		
		ss << _T("</SuperZombieSmash>\n"); //End global tag

		//Write the data to the Document
		tofstream output(pathSaveGame);
		output << ss.str();
		output.close();

		//Make file read-only. Find the handle and it's attributes again to make sure
		//the file still exists
		hFind = FindFirstFile(pathSaveGame.c_str(), &FindFileData);
		dwAttr = GetFileAttributes(pathSaveGame.c_str());

		//if the file exists and we get it's attributes
		//we set the file read-only again to avoid overwriting.
		//User can still disable read-only and change stuff, but we check for invalid
		//nodes and/or attributes
		if(hFind != INVALID_HANDLE_VALUE)
		{
			SetFileAttributes(pathSaveGame.c_str(), dwAttr |= FILE_ATTRIBUTE_READONLY);
		}

		return true;
	}
	return false;
}

bool Serializer::LoadGame(const tstring& pathSaveGame)
{
	if(m_pOwnerGameDirector)
	{
		//Get the Managers, if not present, we can't load a game
		EnemyManager* pEnemyManager = m_pOwnerGameDirector->GetEnemyManager();
		if(pEnemyManager == nullptr)
			return false;

		ScoreManager* pScoreManager = m_pOwnerGameDirector->GetScoreManager();
		if(pScoreManager == nullptr)
			return false;

		ForceFieldManager* pForceFieldManager = m_pOwnerGameDirector->GetForceFieldManager();
		if(pForceFieldManager == nullptr)
			return false;

		//Create a rollbackcontainer on the stack where we store all the data first
		RollBackContainer rbContainer;

		//Create a document on the stack (RAII)
		pugi::xml_document doc;
		//Load a document and see if it has been found
		pugi::xml_parse_result result = doc.load_file(pathSaveGame.c_str());

		//If file not found, return false
		if(!result)
			return false;

		//Root Node
		pugi::xml_node szs = doc.child(_T("SuperZombieSmash"));
		if(szs == nullptr)
			return false;

		//--------------------------------------------------------------------------------
		//Read GameInfo and store it
		if(!LoadGameInfo(szs, rbContainer))
			return false;

		//--------------------------------------------------------------------------------
		//Read Active ScoreObjects and store it
		if(!LoadScoreObjects(szs, rbContainer))
			return false;

		//--------------------------------------------------------------------------------
		//Read Active ForceFieldObjects and store it
		if(!LoadForceFieldObjects(szs, rbContainer))
			return false;

		//--------------------------------------------------------------------------------
		//Read Active Waves and store them
		if(!LoadWaves(szs, rbContainer))
			return false;

		//--------------------------------------------------------------------------------
		//Read Active Enemies, store them and seed all information (also PhysX)
		if(!LoadEnemies(szs, rbContainer))
			return false;

		//--------------------------------------------------------------------------------
		//When we get to this point we must check all managers first and if possible
		//we may clear the scene and insert all new information. If we don't get here we discard
		//all the information that was loaded out of the saveFile!
		if(!LoadSaveGameInMemory(rbContainer))
			return false;

		return true;
	}
	return false;
}

bool Serializer::SaveHighScore(const tstring& pathHighScore)
{
	if(m_pOwnerGameDirector)
	{
		//Before we start writing stuff retrieve handle to file and it's attributes
		DWORD dwAttr;
		WIN32_FIND_DATA FindFileData;
		HANDLE hFind;
		hFind = FindFirstFile(pathHighScore.c_str(), &FindFileData);
		dwAttr = GetFileAttributes(pathHighScore.c_str());

		//if the file exists and it's attributes
		//if the file is read-only we make it writable
		if((hFind != INVALID_HANDLE_VALUE) && (dwAttr & FILE_ATTRIBUTE_READONLY))
		{
			SetFileAttributes(pathHighScore.c_str(), dwAttr &= ~FILE_ATTRIBUTE_READONLY);
		}

		//Write header of the XML
		tstringstream ss;
		ss << _T("<?xml version=\"1.0\"?>\n");

		//Save all the data to the XML file
		ss << _T("<SuperZombieSmash>\n"); //Add global tag

		//-----------------------------------------------
		// HighScore
		//-----------------------------------------------
		ss << _T("\t");
		ss << _T("<HighScore highscore=\"");
		ss << m_pOwnerGameDirector->GetHighScore();
		ss << _T("\"/>\n");

		//----------------------------------------------
		ss << _T("</SuperZombieSmash>\n"); //End global tag

		//Write the data to the Document
		tofstream output(pathHighScore);
		output << ss.str();
		output.close();

		//Make file read-only. Find the handle and it's attributes again to make sure
		//the file still exists
		hFind = FindFirstFile(pathHighScore.c_str(), &FindFileData);
		dwAttr = GetFileAttributes(pathHighScore.c_str());

		//if the file exists and we get it's attributes
		//we set the file read-only again to avoid overwriting.
		//User can still disable read-only and change stuff, but we check for invalid
		//nodes and/or attributes
		if(hFind != INVALID_HANDLE_VALUE)
		{
			SetFileAttributes(pathHighScore.c_str(), dwAttr |= FILE_ATTRIBUTE_READONLY);
		}

		return true;
	}
	return false;
}

bool Serializer::LoadHighScore(const tstring& pathHighScore)
{
	if(m_pOwnerGameDirector)
	{
		//Create a document on the stack (RAII)
		pugi::xml_document doc;
		//Load a document and see if it has been found
		pugi::xml_parse_result result = doc.load_file(pathHighScore.c_str());

		//If file not found, return false
		if(!result)
			return false;

		//Root Node
		pugi::xml_node szs = doc.child(_T("SuperZombieSmash"));
		if(szs == nullptr)
			return false;

		//--------------------------------------------------------------------------------
		//Read HighScore and store it
		pugi::xml_node highScore = szs.child(_T("HighScore"));
		if(highScore == nullptr)
			return false;

		m_pOwnerGameDirector->SetHighScore(highScore.attribute(_T("highscore")).as_uint());

		return true;
	}
	return false;
}

bool Serializer::SerializeGameInfo(tstringstream& ss, int depth)
{
	//Test for managers
	if(m_pOwnerGameDirector == nullptr)
		return false;

	//Score
	for(int i=0; i < depth; ++i)
		ss << _T("\t");

	ss << _T("<Score score=\"");
	ss << m_pOwnerGameDirector->GetCurrentScore();
	ss << _T("\"/>\n");

	//Target Health
	for(int i=0; i < depth; ++i)
		ss << _T("\t");

	Target* currentTarget = m_pOwnerGameDirector->GetCurrentTarget();
	if(currentTarget == nullptr)
		return false;

	ss << _T("<HealthTarget health=\"");
	ss << currentTarget->GetTargetHealth();
	ss << _T("\"/>\n");

	//WaveLevel
	for(int i=0; i < depth; ++i)
		ss << _T("\t");

	ss << _T("<WaveLevel level=\"");
	ss << m_pOwnerGameDirector->GetCurrentWaveLevel();
	ss << _T("\"/>\n");

	//Amount of SmashAllPowers
	for(int i=0; i < depth; ++i)
		ss << _T("\t");

	ss << _T("<SmashAllPowers amount=\"");
	ss << m_pOwnerGameDirector->GetAmountOfSmashAllAbilities();
	ss << _T("\"/>\n");

	//Spawning on Hold
	for(int i=0; i < depth; ++i)
		ss << _T("\t");

	ss << _T("<SpawningState onHold=\"");
	ss << m_pOwnerGameDirector->IsSpawnOnHold();
	ss << _T("\"/>\n");

	return true;
}

bool Serializer::SerializeActiveScoreObjects(tstringstream& ss, int depth)
{
	//Get EnemyManager
	ScoreManager* pScoreManager = m_pOwnerGameDirector->GetScoreManager();

	if(pScoreManager != nullptr)
	{
		//Get the active waves
		vector<ScoreObject*> scoreObjects = pScoreManager->GetActiveScoreObjects();

		for(auto obj : scoreObjects)
		{
			if(obj == nullptr)
				break;

			//Add depth tabs in xml
			for(int i=0; i < depth; ++i)
			{
				ss << _T("\t");
			}

			ss << _T("<ScoreObject currentLifeTime=\"");
			ss << obj->GetCurrentLifeTime(); //the current life time
			ss << _T("\" spawnX=\"");
			ss << obj->GetCurrentPosition().x; //current Position
			ss << _T("\" spawnY=\"");
			ss << obj->GetCurrentPosition().y;
			ss << _T("\" spawnZ=\"");
			ss << obj->GetCurrentPosition().z;
			ss << _T("\"/>\n");
		}
		return true;
	}
	else
		return false;
}

bool Serializer::SerializeActiveForceFields(tstringstream& ss, int depth)
{
	//Get EnemyManager
	ForceFieldManager* pForceFieldManager = m_pOwnerGameDirector->GetForceFieldManager();

	if(pForceFieldManager != nullptr)
	{
		//Get the active waves
		vector<ForceFieldObject*> forceFieldObjects = pForceFieldManager->GetActiveForceFields();

		for(auto forceField : forceFieldObjects)
		{
			if(forceField == nullptr)
				break;

			//Add depth tabs in xml
			for(int i=0; i < depth; ++i)
			{
				ss << _T("\t");
			}

			ss << _T("<ForceFieldObject currentLifeTime=\"");
			ss << forceField->GetCurrentLifeTime(); //the current life time
			ss << _T("\" spawnX=\"");
			ss << forceField->GetCurrentPosition().x; //current Position
			ss << _T("\" spawnY=\"");
			ss << forceField->GetCurrentPosition().y;
			ss << _T("\" spawnZ=\"");
			ss << forceField->GetCurrentPosition().z;
			ss << _T("\" scaleX=\"");
			ss << forceField->GetCurrentScale().x; //current scale
			ss << _T("\" scaleY=\"");
			ss << forceField->GetCurrentScale().y;
			ss << _T("\" scaleZ=\"");
			ss << forceField->GetCurrentScale().z;
			ss << _T("\"/>\n");
		}
		return true;
	}
	else
		return false;
}

bool Serializer::SerializeActiveWaves(tstringstream& ss, int depth)
{
	//Get EnemyManager
	EnemyManager* pEnemyManager = m_pOwnerGameDirector->GetEnemyManager();

	if(pEnemyManager != nullptr)
	{
		//Get the active waves
		vector<EnemyWave*> waves = pEnemyManager->GetEnemyWaves();

		for(auto wave : waves)
		{
			if(wave == nullptr)
				break;

			//Add depth tabs in xml
			for(int i=0; i < depth; ++i)
			{
				ss << _T("\t");
			}

			ss << _T("<Wave lefttospawn=\"");
			ss << wave->GetEnemiesLeftToSpawn(); //Enemies Left to Spawn
			ss << _T("\" spawninterval=\"");
			ss << wave->GetSpawnInterval(); //Spawn Interval
			ss << _T("\" currentinterval=\"");
			ss << wave->GetCurrentIntervalTime(); //Current interval, so it starts where it left off
			ss << _T("\" spawnX=\"");
			ss << wave->GetSpawnLocation().x; //Spawn Position
			ss << _T("\" spawnY=\"");
			ss << wave->GetSpawnLocation().y;
			ss << _T("\" spawnZ=\"");
			ss << wave->GetSpawnLocation().z;
			ss << _T("\"/>\n");
		}
		return true;
	}
	else
		return false;
}

bool Serializer::SerializeActiveEnemies(tstringstream& ss, int depth)
{
	//Get EnemyManager
	EnemyManager* pEnemyManager = m_pOwnerGameDirector->GetEnemyManager();

	if(pEnemyManager != nullptr)
	{
		//Get the active enemies
		vector<Enemy*> enemies = pEnemyManager->GetActiveEnemies();

		for(auto enemy : enemies)
		{
			if(enemy == nullptr)
				break;

			//Add depth tabs in xml
			for(int i=0; i < depth; ++i)
			{
				ss << _T("\t");
			}

			//Open Enemy Tag
			ss << _T("<Enemy type=\"");
			ss << 0; //Add type code here if needed
			ss << _T("\">\n");

			//----------------------------------------
			// States
			//----------------------------------------
			//Add depth tabs in xml
			for(int i=0; i < depth+1; ++i)
			{
				ss << _T("\t");
			}

			ss << _T("<EnemyStates aistate=\"");
			ss << enemy->GetEnemyState();
			ss << _T("\"/>\n");

			//----------------------------------------
			// PhysX Information - IF DEATH OR PARALYZED
			//----------------------------------------
			//Get all actors
			vector<NxActor*> vEnemyRagdollActors = enemy->GetRagdollActors();

			//PhysXStates Tag Start
			for(int i=0; i < depth+1; ++i)
				ss << _T("\t");
			ss << _T("<PhysXStates amountRagdollActors=\"");
			ss << vEnemyRagdollActors.size();
			ss << _T("\">\n");

			//Add Controller Position
			D3DXVECTOR3 controllerPosition = enemy->GetPositionEnemy();

			for(int i=0; i < depth+2; ++i)
				ss << _T("\t");
			ss << _T("<ControllerActorPosition x=\"");
			ss << controllerPosition.x;
			ss << _T("\" y=\"");
			ss << controllerPosition.y;
			ss << _T("\" z=\"");
			ss << controllerPosition.z;
			ss << _T("\"/>\n");

			if(enemy->GetEnemyState() == GameHelper::EnemyState::Dead
				|| enemy->GetEnemyState() == GameHelper::EnemyState::Paralyzed)
			{
				//Serialize actors
				for(auto actor : vEnemyRagdollActors)
				{
					if(actor == nullptr)
						return false;

					//Actor Tag Start
					for(int i=0; i < depth+2; ++i)
						ss << _T("\t");
					ss << _T("<RagdollActor>\n");

					//Actor POSE tag
					D3DXMATRIX matGlobalPose;
					D3DXMatrixIdentity(&matGlobalPose);
					PhysicsManager::GetInstance()->NMatToDMat(matGlobalPose, 
						actor->getGlobalPose());

					for(int i=0; i < depth+3; ++i)
						ss << _T("\t");
					ss << _T("<GlobalPose _11=\"");
					ss << matGlobalPose._11;
					ss << _T("\" _12=\"");
					ss << matGlobalPose._12;
					ss << _T("\" _13=\"");
					ss << matGlobalPose._13;
					ss << _T("\" _14=\"");
					ss << matGlobalPose._14;
					ss << _T("\" _21=\"");
					ss << matGlobalPose._21;
					ss << _T("\" _22=\"");
					ss << matGlobalPose._22;
					ss << _T("\" _23=\"");
					ss << matGlobalPose._23;
					ss << _T("\" _24=\"");
					ss << matGlobalPose._24;
					ss << _T("\" _31=\"");
					ss << matGlobalPose._31;
					ss << _T("\" _32=\"");
					ss << matGlobalPose._32;
					ss << _T("\" _33=\"");
					ss << matGlobalPose._33;
					ss << _T("\" _34=\"");
					ss << matGlobalPose._34;
					ss << _T("\" _41=\"");
					ss << matGlobalPose._41;
					ss << _T("\" _42=\"");
					ss << matGlobalPose._42;
					ss << _T("\" _43=\"");
					ss << matGlobalPose._43;
					ss << _T("\" _44=\"");
					ss << matGlobalPose._44;
					ss << _T("\"/>\n");

					//Actor LINVEL tag
					NxVec3 linVel = actor->getLinearVelocity();
					for(int i=0; i < depth+3; ++i)
						ss << _T("\t");
					ss << _T("<LinearVelocity x=\"");
					ss << linVel.x;
					ss << _T("\" y=\"");
					ss << linVel.y;
					ss << _T("\" z=\"");
					ss << linVel.z;
					ss << _T("\"/>\n");

					//Actor Tag End
					for(int i=0; i < depth+2; ++i)
						ss << _T("\t");
					ss << _T("</RagdollActor>\n");
				}
			}
			//PhysXStates Tag End
			for(int i=0; i < depth+1; ++i)
				ss << _T("\t");
			ss << _T("</PhysXStates>\n");
		

			//----------------------------------------
			//Add depth tabs in xml
			for(int i=0; i < depth; ++i)
			{
				ss << _T("\t");
			}
			//Close Enemy Tag
			ss << _T("</Enemy>\n");
		}
		return true;
	}
	else
		return false;
}

bool Serializer::LoadGameInfo(pugi::xml_node root, RollBackContainer& rollbackContainer)
{
	if(root != nullptr)
	{
		pugi::xml_node gameInfo = root.child(_T("GameInfo"));
		if(gameInfo == nullptr)
			return false;
	
		//Score
		pugi::xml_node score = gameInfo.child(_T("Score"));
		if(score == nullptr)
			return false;

		rollbackContainer.SetScore(score.attribute(_T("score")).as_uint());

		//Target health
		pugi::xml_node health = gameInfo.child(_T("HealthTarget"));
		if(health == nullptr)
			return false;

		rollbackContainer.SetTargetHealth(health.attribute(_T("health")).as_float());

		//Wave level
		pugi::xml_node waveLevel = gameInfo.child(_T("WaveLevel"));
		if(waveLevel == nullptr)
			return false;

		rollbackContainer.SetWaveLevel(waveLevel.attribute(_T("level")).as_uint());

		//SmashAllAbilities
		pugi::xml_node amountSmashAll = gameInfo.child(_T("SmashAllPowers"));
		if(amountSmashAll == nullptr)
			return false;

		rollbackContainer.SetAmountSmashAllPowers(amountSmashAll.attribute(_T("amount")).as_uint());

		//SpawnStates
		pugi::xml_node spawnState = gameInfo.child(_T("SpawningState"));
		if(spawnState == nullptr)
			return false;

		rollbackContainer.SetSpawningOnHold(spawnState.attribute(_T("onHold")).as_bool());

		return true;
	}
	return false;
}

bool Serializer::LoadScoreObjects(pugi::xml_node root, RollBackContainer& rollbackContainer)
{
	if(root != nullptr)
	{
		//Get the ScoreManager
		ScoreManager* pScoreManager = m_pOwnerGameDirector->GetScoreManager();
		if(pScoreManager == nullptr)
			return false;

		pugi::xml_node activeScoreObjects = root.child(_T("ActiveScoreObjects"));
		if(activeScoreObjects == nullptr)
			return false;

		for(pugi::xml_node node = activeScoreObjects.child(_T("ScoreObject")); node != nullptr; 
			node = node.next_sibling(_T("ScoreObject")))
		{
			if(node == nullptr)
				return false;

			//Create local score object variable
			RollBackScore rbScore;

			//Store the position in the container
			D3DXVECTOR3 pos;
			pos.x = node.attribute(_T("spawnX")).as_float();
			pos.y = node.attribute(_T("spawnY")).as_float();
			pos.z = node.attribute(_T("spawnZ")).as_float();
			
			rbScore.position = pos;
			rbScore.currentLifeTime = node.attribute(_T("currentLifeTime")).as_float();

			//Add the wave to the container == copy
			rollbackContainer.AddRollBackScoreObject(rbScore);
		}
		return true;
	}
	return false;
}

bool Serializer::LoadForceFieldObjects(pugi::xml_node root, RollBackContainer& rollbackContainer)
{
	if(root != nullptr)
	{
		//Get the ForceFieldManager
		ForceFieldManager* pForceFieldManager = m_pOwnerGameDirector->GetForceFieldManager();
		if(pForceFieldManager == nullptr)
			return false;

		pugi::xml_node activeForceFieldObjects = root.child(_T("ActiveForceFieldObjects"));
		if(activeForceFieldObjects == nullptr)
			return false;

		for(pugi::xml_node node = activeForceFieldObjects.child(_T("ForceFieldObject")); node != nullptr; 
			node = node.next_sibling(_T("ForceFieldObject")))
		{
			if(node == nullptr)
				return false;

			//Create local score object variable
			RollBackForceField rbForceField;

			//Store the position in the container
			D3DXVECTOR3 pos;
			pos.x = node.attribute(_T("spawnX")).as_float();
			pos.y = node.attribute(_T("spawnY")).as_float();
			pos.z = node.attribute(_T("spawnZ")).as_float();

			D3DXVECTOR3 scale;
			scale.x = node.attribute(_T("scaleX")).as_float();
			scale.y = node.attribute(_T("scaleY")).as_float();
			scale.z = node.attribute(_T("scaleZ")).as_float();
			
			rbForceField.position = pos;
			rbForceField.scale = scale;
			rbForceField.currentLifeTime = node.attribute(_T("currentLifeTime")).as_float();

			//Add the wave to the container == copy
			rollbackContainer.AddForceFieldObject(rbForceField);
		}
		return true;
	}
	return false;
}

bool Serializer::LoadWaves(pugi::xml_node root, RollBackContainer& rollbackContainer)
{
	if(root != nullptr)
	{
		//Get the EnemyManager
		EnemyManager* pEnemyManager = m_pOwnerGameDirector->GetEnemyManager();
		if(pEnemyManager == nullptr)
			return false;

		pugi::xml_node activeWaves = root.child(_T("ActiveWaves"));
		if(activeWaves == nullptr)
			return false;

		for(pugi::xml_node node = activeWaves.child(_T("Wave")); node != nullptr; node = node.next_sibling(_T("Wave")))
		{
			if(node == nullptr)
				return false;

			//Create local wave variable
			RollBackWave rbWave;

			//Store the position in the container
			D3DXVECTOR3 pos;
			pos.x = node.attribute(_T("spawnX")).as_float();
			pos.y = node.attribute(_T("spawnY")).as_float();
			pos.z = node.attribute(_T("spawnZ")).as_float();
			
			rbWave.spawnPosition = pos;
			rbWave.enemiesLeftToSpawn = node.attribute(_T("lefttospawn")).as_uint();
			rbWave.spawnInterval = node.attribute(_T("spawninterval")).as_float();
			rbWave.currentInterval = node.attribute(_T("currentinterval")).as_float();

			//Add the wave to the container == copy
			rollbackContainer.AddRollBackWave(rbWave);
		}
		return true;
	}
	return false;
}

bool Serializer::LoadEnemies(pugi::xml_node root, RollBackContainer& rollbackContainer)
{
	if(root != nullptr)
	{
		pugi::xml_node activeEnemies = root.child(_T("ActiveEnemies"));
		if(activeEnemies == nullptr)
			return false;

		for(pugi::xml_node node = activeEnemies.child(_T("Enemy")); node != nullptr; node = node.next_sibling(_T("Enemy")))
		{
			if(node == nullptr)
				return false;

			//Create local rollback enemy variable
			RollBackEnemy rbEnemy;

			//Retrieve the type as attribute if needed
			int enemyType = -1;
			enemyType = node.attribute(_T("type")).as_int();

			//Get the EnemyStates + attributes
			pugi::xml_node enemyState = node.child(_T("EnemyStates"));
			if(enemyState == nullptr)
				return false;

			int aiState = enemyState.attribute(_T("aistate")).as_int();
			rbEnemy.enemyState = static_cast<GameHelper::EnemyState>(aiState);

			//Get the PhysXStates + attributes if needed
			pugi::xml_node physxState = node.child(_T("PhysXStates"));
			if(physxState == nullptr)
				return false;
			
			//Load all the actors depending on the enemy type
			bool succeeded = true;
			switch(enemyType)
			{
			case 0:
				succeeded = LoadEnemyType0(physxState, rollbackContainer, rbEnemy, 11); //Load Enemy 0, which uses 11 ragdollactors
				if(succeeded == false) //If something failed return false
					return false;
				break;
			default:
				//nothing
				break;
			}
		}
		return true;
	}
	return false;
}

bool Serializer::LoadEnemyType0(pugi::xml_node physXStatesEnemy, RollBackContainer& rollbackContainer, 
								RollBackEnemy& rbEnemy, UINT amountOfRagdollActors)
{
	//Go through all the actor nodes and retrieve their globalPose and linear velocity
	//IF and ONLY IF we are in the seed state!
	//If in leech state we only read the character controller information. There won't be
	//ragdoll actor information in the savefile.
	if(physXStatesEnemy == nullptr)
		return false;

	//Get the current state of our enemy that has allready been set with the savegame data
	GameHelper::EnemyState aiState = rbEnemy.enemyState;

	//Type is important because our enemy creates actors for it's ragdoll. We want to fill
	//in these actors with our savegame information.
	//We must be sure we've got the right type and same amount of actors!!!
	rbEnemy.amountOfRagdollActors = physXStatesEnemy.attribute(_T("amountRagdollActors")).as_int();
	if(rbEnemy.amountOfRagdollActors != amountOfRagdollActors)
		return false;

	//If our enemy is dead are paralyzed we set the state of the ragdoll and recieve the information
	//for the controller and the ragdollactors
	if(rbEnemy.enemyState == GameHelper::EnemyState::Paralyzed ||
		rbEnemy.enemyState == GameHelper::EnemyState::Dead)
	{
		//Prepare our enemy to seed by setting it's state
		//Internal we handle checking if things must be prepared or not :)
		rbEnemy.ragdollState = RagdollState::SeedState;

		//Read in controller information + pass it
		pugi::xml_node controllerNode = physXStatesEnemy.child(_T("ControllerActorPosition"));
		if(controllerNode == nullptr)
			return false;

		D3DXVECTOR3 controllerPosition;
		controllerPosition.x = controllerNode.attribute(_T("x")).as_float();
		controllerPosition.y = controllerNode.attribute(_T("y")).as_float();
		controllerPosition.z = controllerNode.attribute(_T("z")).as_float();

		//enemy->SetPositionEnemy(controllerPosition);
		rbEnemy.positionController = controllerPosition;

		//Go trough our nodes and retrieve data
		for(pugi::xml_node actorNode = physXStatesEnemy.child(_T("RagdollActor")); actorNode != nullptr; 
			actorNode = actorNode.next_sibling(_T("RagdollActor")))
		{
			if(actorNode == nullptr)
				return false;

			//------------------------------------------------------------------------------
			//Set the GlobalPose
			pugi::xml_node globalPoseNode = actorNode.child(_T("GlobalPose"));
			if(globalPoseNode == nullptr)
				return false;

			D3DXMATRIX dmatGlobalPose;
			dmatGlobalPose._11 = globalPoseNode.attribute(_T("_11")).as_float();
			dmatGlobalPose._12 = globalPoseNode.attribute(_T("_12")).as_float();
			dmatGlobalPose._13 = globalPoseNode.attribute(_T("_13")).as_float();
			dmatGlobalPose._14 = globalPoseNode.attribute(_T("_14")).as_float();

			dmatGlobalPose._21 = globalPoseNode.attribute(_T("_21")).as_float();
			dmatGlobalPose._22 = globalPoseNode.attribute(_T("_22")).as_float();
			dmatGlobalPose._23 = globalPoseNode.attribute(_T("_23")).as_float();
			dmatGlobalPose._24 = globalPoseNode.attribute(_T("_24")).as_float();

			dmatGlobalPose._31 = globalPoseNode.attribute(_T("_31")).as_float();
			dmatGlobalPose._32 = globalPoseNode.attribute(_T("_32")).as_float();
			dmatGlobalPose._33 = globalPoseNode.attribute(_T("_33")).as_float();
			dmatGlobalPose._34 = globalPoseNode.attribute(_T("_34")).as_float();

			dmatGlobalPose._41 = globalPoseNode.attribute(_T("_41")).as_float();
			dmatGlobalPose._42 = globalPoseNode.attribute(_T("_42")).as_float();
			dmatGlobalPose._43 = globalPoseNode.attribute(_T("_43")).as_float();
			dmatGlobalPose._44 = globalPoseNode.attribute(_T("_44")).as_float();

			//Store it for the actor
			NxMat34 nmatGlobalPose;
			PhysicsManager::GetInstance()->DMatToNMat(nmatGlobalPose, dmatGlobalPose);
			rbEnemy.ragdollActorTransforms.push_back(nmatGlobalPose);

			//------------------------------------------------------------------------------
			//Set the Linear Velocity
			pugi::xml_node linVelNode = actorNode.child(_T("LinearVelocity"));
			if(linVelNode == nullptr)
				return false;
				
			NxVec3 linVel = NxVec3(0,0,0);
			linVel.x = linVelNode.attribute(_T("x")).as_float();
			linVel.y = linVelNode.attribute(_T("y")).as_float();
			linVel.z = linVelNode.attribute(_T("z")).as_float();

			//Store it for actor
			rbEnemy.ragdollActorLinVel.push_back(linVel);
		}
	}
	//ELSE we ragdoll state to leech and fill in the position of our controller
	else
	{
		//Prepare our enemy to leech by setting it's state
		//Internal we handle checking if things must be prepared or not :)
		rbEnemy.ragdollState = RagdollState::LeechState;

		//Read in controller information + pass it
		pugi::xml_node controllerNode = physXStatesEnemy.child(_T("ControllerActorPosition"));
		if(controllerNode == nullptr)
			return false;

		D3DXVECTOR3 controllerPosition;
		controllerPosition.x = controllerNode.attribute(_T("x")).as_float();
		controllerPosition.y = controllerNode.attribute(_T("y")).as_float();
		controllerPosition.z = controllerNode.attribute(_T("z")).as_float();

		rbEnemy.positionController = controllerPosition;
	}

	//----------------------------------------------------------------------------
	//Double check in the end that the amount of actors that must be filled
	//equals the amount of transforms stored IF AND ONLY IF Ragdoll is seed state
	if(rbEnemy.ragdollState == RagdollState::SeedState)
	{
		if(rbEnemy.ragdollActorTransforms.size() != rbEnemy.amountOfRagdollActors
			|| rbEnemy.ragdollActorLinVel.size() != rbEnemy.amountOfRagdollActors)
			return false;
	}

	//Add the enemy to the container
	rollbackContainer.AddRollBackEnemy(rbEnemy);
	return true;
}

bool Serializer::LoadSaveGameInMemory(const RollBackContainer& rollbackContainer)
{
	//If we get in this function we check all the managers needed before we continue
	if(m_pOwnerGameDirector == nullptr)
		return false;

	EnemyManager* pEnemyManager = m_pOwnerGameDirector->GetEnemyManager();
	if(pEnemyManager == nullptr)
		return false;

	ScoreManager* pScoreManager = m_pOwnerGameDirector->GetScoreManager();
	if(pScoreManager == nullptr)
		return false;

	ForceFieldManager* pForceFieldManager = m_pOwnerGameDirector->GetForceFieldManager();
	if(pForceFieldManager == nullptr)
		return false;

	//Clear Scene
	m_pOwnerGameDirector->NewGame();

	//Load All GameInfo
	m_pOwnerGameDirector->SetCurrentScore(rollbackContainer.GetScore());
	m_pOwnerGameDirector->SetCurrentWaveLevel(rollbackContainer.GetWaveLevel());
	m_pOwnerGameDirector->SetAmountOfSmashAllAbilities(rollbackContainer.GetAmountSmashAllPowers());
	m_pOwnerGameDirector->SetSpawnOnHold(rollbackContainer.IsSpawningOnHold());

	//Load All ScoreObjects
	for(auto scoreObj : rollbackContainer.GetScoreObjects())
	{
		ScoreObject* spawnedStar = pScoreManager->SpawnScoreStar(scoreObj.position);

		if(spawnedStar == nullptr)
			break;

		spawnedStar->SetCurrentLifeTime(scoreObj.currentLifeTime);
	}
	
	//Load All ForceFieldObjects
	for(auto forceFieldObj : rollbackContainer.GetForceFieldObjects())
	{
		//We invoke it via the GameDirector in this sample because there are happening some game condition
		//chances with our force fields. If the forcefields would differ we need to introduce an id +
		//work via the forcefieldmanager. This is just coded this way to showcase forcefields.
		ForceFieldObject* forceField = m_pOwnerGameDirector->InvokeSmashAllAbility(forceFieldObj.groupID, 
			forceFieldObj.position, forceFieldObj.scale);

		if(forceField == nullptr)
			break;

		forceField->SetCurrentLifeTime(forceFieldObj.currentLifeTime);
	}

	//Load All Waves
	for(auto wave : rollbackContainer.GetWaves())
	{
		pEnemyManager->CreateWave(wave.enemiesLeftToSpawn, wave.spawnInterval, wave.spawnPosition,
			wave.currentInterval);
	}

	//Load All Enemies
	for(auto enemy : rollbackContainer.GetEnemies())
	{
		//Create an enemy
		Enemy* memEnemy = pEnemyManager->SpawnEnemy();

		if(memEnemy == nullptr)
			break;

		//Retrieve the current target of the current game because it is bound to a level, not an enemy itself
		memEnemy->SetTarget(m_pOwnerGameDirector->GetCurrentTarget());

		//Set the state of the enemy
		memEnemy->SetEnemyState(enemy.enemyState);

		//If he is dead flag him for removal allready
		if(memEnemy->GetEnemyState() == GameHelper::EnemyState::Dead)
			pEnemyManager->FlagEnemyForRemoval(memEnemy);

		//Get the actors
		vector<NxActor*> vEnemyRagdollActors = memEnemy->GetRagdollActors();

		//Amount of actors allready checked before this stage. Else rollback can't be done our way!

		//Set ragdoll state
		memEnemy->SetRagdollState(enemy.ragdollState);

		//Set the controller of the enemy
		memEnemy->SetPositionEnemy(enemy.positionController);

		//Go through all the actors and position them IF AND ONLY IF ragdoll is seeding
		//Else there won't be data in the file. See the LoadEnemy0 for the check!
		if(memEnemy->GetRagdollState() == RagdollState::SeedState)
		{
			//Counter
			UINT actorID = 0;

			for(auto actor : vEnemyRagdollActors)
			{
				if(actor != nullptr)
				{
					actor->setGlobalPose(enemy.ragdollActorTransforms.at(actorID));
					actor->setLinearVelocity(enemy.ragdollActorLinVel.at(actorID));
				}

				++actorID;
			}
		}
	}
	return true;
}