#include "stdafx.h"
#include "Game.h"
#include "PlayerManager.h"
#include "Pawn.h"
#include "ArcaStone.h"
#include "ClientManager.h"
#include "ClientSession.h"

#define START_POINT_PLAYER1 Coord(3,5)
#define START_POINT_PLAYER2 Coord(3,1)


Game::Game(GameNumber gameNum) : m_GameNum(gameNum)
{
}

Game::~Game()
{
	for (auto unit : m_UnitList)
	{
		delete unit;
	}
	m_PlayerList.clear();
	m_UnitList.clear();
}

void Game::InitGame(PlayerNumber player1, PlayerNumber player2)
{
	m_UnitIdentityNumberCounter = 0;
	m_PlayerList.reserve(2);
	m_PlayerList.push_back(player1);
	m_PlayerList.push_back(player2);

	// initialize field
	m_GameField.InitField(MAP_FIELD_WIDTH, MAP_FIELD_HEIGHT); // 7 by 10's rectangle field, like ipad prototype

	// create and initialize unit
	// get group data from each player
	for (auto playerNumber : m_PlayerList)
	{
		auto player = GPlayerManager->GetPlayer(playerNumber);
		auto group = player->GetGroupList()[0];

		// get unit data from group
		for (auto it = group.m_UnitDataList.begin(); it != group.m_UnitDataList.end(); it++)
		{
			auto unitData = it->second;
			auto originPosition = it->first;
			Unit* unit = nullptr;

			switch (unitData.m_UnitType)
			{
			case UT_PAWN:
			{
				unit = new Pawn();

			}break;
			}
			assert(unit != nullptr);

			// init by copying unitData
			unit->InitUnit(unitData, playerNumber, GenerateUnitIdentityNumber());
			Coord position;

			if (playerNumber == player1)
			{
				position = START_POINT_PLAYER1 + originPosition; // locate unit by group data
			}
			else if (playerNumber == player2)
			{
				position = START_POINT_PLAYER2 - originPosition; // 대칭으로
			}
			unit->SetPosition(position);
			m_UnitList.push_back(unit);
		};
	}

	// 아르카스톤 설치
	if (USE_ARCA) SetUpArca();

	// 유닛 수 초기화
	UnitCounting();
	
	// play turn and first attacker setting
	m_Attacker = m_PlayerList.at(rand() % m_PlayerList.size());
	m_CanCommand = MAX_TURN;
	m_MaxTurn[0] = MAX_TURN;
	m_MaxTurn[1] = MAX_TURN;
	m_PlayTurn = 0;
	m_Winner = WW_NONE;
	m_IsFirstTurn = true;
}

void Game::HandleAttack(PlayerNumber attacker, AttackData attackData)
{
	if (m_Attacker != attacker) // 아직 턴이 아닌데 공격을 시도하면
	{
		//무시
		return;
	}

	// 그 외에 여러가지 공격조건에 만족하지 못한다면
	if (!IsCorrectAttack(attackData))
	{
		// 무시!
		return;
	}

	m_UnitActionQueue.clear();

	Unit* attackUnit = GetUnit(attackData.id);
	// 유닛 이동 시작!
	UnitMove(attackData.direction, attackData.Range, attackUnit, true);	// 함수 내에서 유닛들 데굴데굴 구루는중~

	// 이동했으니까 이동 가능 횟수 - 1
	m_CanCommand--;

	// 유닛이 어떻게 어떻게 이동했는지 통째로 알려준다!
	{
		Packet::AttackResult outPacket;
		outPacket.mQueueLength = m_UnitActionQueue.size();
		for (unsigned int i = 0; i < m_UnitActionQueue.size(); i++)
		{
			memcpy(&outPacket.mUnitActionQueue[i], &m_UnitActionQueue[i], sizeof(UnitAction));
		}
		for (auto playerNumber : m_PlayerList)
		{
			auto session = GClientManager->GetClient(playerNumber);
			if (session != nullptr)
				session->SendRequest(&outPacket);
		}
	}
	
	IsGameOver();
	if (m_IsGameOver)
	{
		// 게임이 끝났으면 승자 알려줌
		Packet::GameOverResult outPacket;
		for (auto playerNumber : m_PlayerList)
		{
			outPacket.mWhoIsWinner = m_Winner;

			auto session = GClientManager->GetClient(playerNumber);
			if (session != nullptr)
				session->SendRequest(&outPacket);
		}
		return;
	}

	// TODO : 플레이어의 남은 이동 가능 횟수 패킷 전달

	IsArca();	// 아르카스톤에 대한 턴 처리 해주고..

	m_PlayTurn++;	// 턴 경과요~
	
	// TODO : 남은 턴 수, 최대 턴 수 패킷 전달

	// 남은 턴 횟수가 없다면
	if (m_CanCommand <= 0)
	{
		m_IsFirstTurn = false;

		if (m_Attacker == m_PlayerList[0])
		{
			// 공격자를 바꾸고
			m_Attacker = m_PlayerList[1];
			// 얼마나 움직일 수 있는지 알려준다.
			m_CanCommand = m_MaxTurn[1];
			// 플레이어의 마나량을 재밍하는 스킬을 넣고싶다면 이 값에 일정값을 빼주면 된다.
		}
		else
		{
			m_Attacker = m_PlayerList[0];
			m_CanCommand = m_MaxTurn[0];
		}

		// 공격하라는 신호를 보낸다!
		{
			Packet::YourTurnResult outPacket;
			for (auto playerNumber : m_PlayerList)
			{
				if (m_Attacker == playerNumber)
					outPacket.mIsReallyMyTurn = true;
				else
					outPacket.mIsReallyMyTurn = false;

				auto session = GClientManager->GetClient(playerNumber);
				if (session != nullptr)
					session->SendRequest(&outPacket);
			}
		}
	}
}
void Game::UnitMove(HexaDirection direction, int range, Unit* unit, bool isFirstMove)
{
	if (range <= 0)
	{
		// finish Move
		return;
	}

	// 처음 부딪힌 유닛과, 그 라인에서 마지막으로 연계되어 부딪힐 유닛을 찾는다.
	Unit* firstGuy = nullptr;
	Unit* lastGuy = nullptr;
	int costLength = -1;
	for (int l = 1; l <= range; l++)
	{
		// unit의 direction 방향으로 l만큼 거리에 서있는 유닛
		Unit* standGuy = GetUnitInPosition(unit->GetPos() + GetUnitVector(direction) * l);
		if (standGuy == nullptr) // 게 서있는가? - 아뇨
		{
			if (firstGuy == nullptr) // 처음으로 만난 놈이 아직 없는가?
				continue;
			else // 이제껏 충돌은 하긴 했는가? 그럼 충돌을 멈춰야겠구려
				break;
		}
		else{ // 네놈, 거기 서있었구나!
			if (firstGuy == nullptr) // 아직 아무도 만나지 못했는가?
			{
				costLength = l;
				firstGuy = standGuy;
			}
			lastGuy = standGuy;
		}
	}

	if (firstGuy != nullptr) // 누군가과 (입술)박치기를 했습니까?
	{
		// 가장 가까이에서 만난 애의 한칸 뒤에 유닛을 배치하고
		unit->SetPosition(unit->GetPos() + GetUnitVector(direction) * (costLength - 1));

		UnitAction action;
		action.mActionType = UAT_MOVE;
		action.mUnitId = unit->GetID();
		action.mMoveData.mRange = costLength - 1;
		action.mMoveData.mDirection = direction;
		action.mMoveData.mFinalX = unit->GetPos().x;
		action.mMoveData.mFinalY = unit->GetPos().y;
		m_UnitActionQueue.push_back(action);


		if (m_GameField.isInsideOfField(unit->GetPos()) == false) // 거 유닛 바깥에 나갔슴까?
			KillThisUnit(unit); //그럼 젠뷰쌰쓰!

		UnitPush(unit, lastGuy, range - costLength, isFirstMove); // 맨 마지막에 있는 놈만 밀어버리세요.
	}
	else // 당신 앞에 아무도 없다면.. 크큭 솔로군
	{
		unit->SetPosition(unit->GetPos() + GetUnitVector(direction) * range); // 쭉 앞으로 가시어요

		UnitAction action;
		action.mActionType = UAT_MOVE;
		action.mUnitId = unit->GetID();
		action.mMoveData.mRange = range;
		action.mMoveData.mDirection = direction;
		action.mMoveData.mFinalX = unit->GetPos().x;
		action.mMoveData.mFinalY = unit->GetPos().y;
		m_UnitActionQueue.push_back(action);

		if (m_GameField.isInsideOfField(unit->GetPos()) == false) // 거 유닛 바깥에 나갔슴까?
			KillThisUnit(unit); //그럼 젠뷰쌰쓰!.. 안그래도 혼잔데 낙사라니 ㅠㅠ
	}
}

void Game::UnitPush(Unit* pusher, Unit* target, int power, bool isFirstPush)
{
	if (isFirstPush) // 첫 충돌이면 공격력에 비례해서 밀고
		UnitMove(GetHexaDirection(pusher->GetPos(), target->GetPos()), pusher->GetAttack() - target->GetWeight(), target, false);
	else // 두번째 이상의 충돌이면 이제까지의 밀린 정도를 감안해서 밀고
		UnitMove(GetHexaDirection(pusher->GetPos(), target->GetPos()), power - target->GetWeight(), target, false);

	//적이면 데미지 먹이고
	if (pusher->GetOwner() != target->GetOwner())
		UnitApplyDamageWithCollision(target, pusher);
	else
	{
		UnitAction action;
		action.mActionType = UAT_COLLISION;
		action.mUnitId = pusher->GetID();
		action.mCollisionData.mTarget = target->GetID();
		action.mCollisionData.mMyHP = pusher->GetHP();
		action.mCollisionData.mTargetHP = target->GetHP();
		m_UnitActionQueue.push_back(action);
	}
}

void Game::UnitApplyDamageWithCollision(Unit* thisGuy, Unit* thatGuy)
{
	thisGuy->SetHP(thisGuy->GetHP() - thatGuy->GetAttack());
	thatGuy->SetHP(thatGuy->GetHP() - thisGuy->GetAttack());

	UnitAction action;
	action.mActionType = UAT_COLLISION;
	action.mUnitId = thisGuy->GetID();
	action.mCollisionData.mTarget = thatGuy->GetID();
	action.mCollisionData.mMyHP = thisGuy->GetHP();
	action.mCollisionData.mTargetHP = thatGuy->GetHP();
	m_UnitActionQueue.push_back(action);

	if (thisGuy->GetHP() <= 0)
		KillThisUnit(thisGuy);
	if (thatGuy->GetHP() <= 0)
		KillThisUnit(thatGuy);
}

void Game::KillThisUnit(Unit* unit)
{
	// TODO

	// Temp Code
	unit->SetPosition(Coord(INT_MAX, INT_MAX));
	unit->setStatus(UST_DEAD);

	UnitAction action;
	action.mActionType = UAT_DIE;
	action.mUnitId = unit->GetID();
	m_UnitActionQueue.push_back(action);
}

void Game::StartGame()
{
	int random = rand() % m_PlayerList.size();
	m_Attacker = m_PlayerList[random];
	
	Packet::YourTurnResult outPacket;
	for (auto playerNumber : m_PlayerList)
	{
		if (playerNumber == m_Attacker)
			outPacket.mIsReallyMyTurn = true;
		else
			outPacket.mIsReallyMyTurn = false;
		auto session = GClientManager->GetClient(playerNumber);
		if (session != nullptr)
			session->SendRequest(&outPacket);
	}
}

void Game::IsArca()
{
	// 옆에 알카스톤이 있는지 보고, 전에 없었는데 이번에 있으면 이동가능횟수++
	// 반대면 --

	// 첫번쨰 턴이면 알카스톤적용X
	if (m_IsFirstTurn)
		return;
	
	if (!USE_ARCA)
		return;

	bool isNearAlkaStone = false;
	ArcaStone* arcaStone = nullptr;

	// 아르카스톤 찾고
	for (auto unit : m_UnitList)
	{
		if (unit->GetUnitType() == UT_ARCASTONE)
		{
			arcaStone = dynamic_cast<ArcaStone*>(unit);
			break;
		}
	}
	// 읭 아르카스톤 없는데여?
	if (arcaStone == nullptr)
	{
		// 그럼 그냥 지나가
		return;
	}

	for (Unit* unit : m_UnitList)
	{
		//내 유닛이 아르카 스톤 옆에 있습니까?
		if (unit->GetOwner() == m_Attacker)
		{
			for (int i = 1; i <= 6; i++) // Itor for HexaDirection
			{
				Coord positionGap = Coord(arcaStone->GetPos().x - unit->GetPos().x, arcaStone->GetPos().y - unit->GetPos().y);
				// 처리
				if (positionGap == GetUnitVector((HexaDirection)i))
				{
					isNearAlkaStone = true;
					goto finishFindUnitNearArcastone;
				}
			}
		}
	}
finishFindUnitNearArcastone:

	int whosTurn;
	if (m_Attacker == m_PlayerList.at(0))
		whosTurn = 0;
	else whosTurn = 1;

	if (isNearAlkaStone)	// 이번 턴에 아르카스톤이 옆에 있고
	{
		if (m_IsNearArca[whosTurn])	// 전 턴에도 옆에 있었다면
		{
			// 알게뭐람
		}
		else				// 전 턴에는 옆에 없었다면
		{
			// 최대 이동가능 횟수를 1 증가시킨다.
			m_MaxTurn[whosTurn]++;
			// 현재 이동가능 횟수도 1 증가시킨다.
			m_CanCommand++;
			// 그대 곁에 아르카스톤이 있으라
			m_IsNearArca[whosTurn] = true;
		}
	}
	else					// 이번 턴에 아르카스톤이 옆에 없고
	{
		if (m_IsNearArca[whosTurn])	// 전 턴에는 옆에 있었다면
		{
			m_MaxTurn[whosTurn]--;
			m_CanCommand--;
			m_IsNearArca[whosTurn] = false;
		}
		else				// 전 턴에도 옆에 없었다면
		{
			// 무시
		}
	}
}

void Game::IsGameOver()
{
	// 유닛 카운팅!
	UnitCounting();

	// m_IsGameOver = false; 게임이 종료되지 않은 상태에서 이 함수가 실행됬다고 가정 .

	// 플레이어1 전멸이요~
	if (m_LivingUnitCount[0] == 0)
	{
		// 그럼 게임을 종료시켜야지!
		m_IsGameOver = true;
		m_Winner = WW_PLAYER1;
	}

	// 플레이어2 전멸이요~
	if (m_LivingUnitCount[1] == 0)
	{
		m_Winner = WW_PLAYER2;
		// 플레이어1도 전멸인디요? 그럼 누가 이긴거야?
		if (m_IsGameOver)
		{
			// 무승부가 있는 게임이라면 무승부지!
			if (USE_DRAW)
			{
				m_Winner = WW_DRAW;
			}
			// 아니면 마지막에 공격한 사람이 이긴거지!
			else
			{
				// 플레이어1이 공격자래매요?
				if (m_PlayerList.at(0) == m_Attacker)
				{
					m_Winner = WW_PLAYER1;
				}
				// 아니라면 플레이어2가 이긴거군
				else
				{
					m_Winner = WW_PLAYER2;
				}
			}
		}
		m_IsGameOver = true;
	}
}

void Game::SetUpArca()
{
	// create arcastone(npc)
	ArcaStone* arcaStone = new ArcaStone();

	arcaStone->SetOwner(PLAYER_NUMBER_NPC);
	arcaStone->SetPosition(Coord(3, 5)); // Center of Map
	m_UnitList.push_back(arcaStone);
}

void Game::UnitCounting()
{
	for (int i = 0; i < 3; ++i)
	{
		// 0으로 초기화
		m_UnitCount[i] = 0;
		m_LivingUnitCount[i] = 0;
	}

	for (Unit* unit : m_UnitList)
	{
		// 살아있는 유닛
		if (unit->GetUnitStatus() != UST_DEAD)
		{
			if (unit->GetOwner() == m_PlayerList.at(0))
			{
				// 플레이어1 유닛 하나요~
				m_UnitCount[0]++;
				m_LivingUnitCount[0]++;
			}
			else if (unit->GetOwner() == m_PlayerList.at(1))
			{
				m_UnitCount[1]++;
				m_LivingUnitCount[1]++;
			}
			else
			{
				// NPC 의 유닛수도 세주자. 필요할진 모르겠당!
				m_UnitCount[2]++;
				m_LivingUnitCount[2]++;
			}
		}
		// 죽은 유닛
		else
		{
			if (unit->GetOwner() == m_PlayerList.at(0))
			{
				m_UnitCount[0]++;
			}
			else if (unit->GetOwner() == m_PlayerList.at(1))
			{
				m_UnitCount[1]++;
			}
			else
			{
				m_UnitCount[2]++;
			}
		}
	}
}

bool Game::IsCorrectAttack(AttackData attackData)
{
	if (m_IsGameOver)		// 게임이 끝났는데 무슨 공격이야!
	{
		return false; // 무시!
	}

	if (GetUnit(attackData.id) == nullptr)// 잘못된 유닛으로 공격하려고 햇으니까
	{
		//무시해!
		return false;
	}

	//공격 데이터에 문제가 있으면
	if (attackData.Range <= 0)
	{
		// TODO : 공격이 합당한 공격인지 확인하는 코드
		//무시
		return false;
	}

	// 올바른 공격이군!
	return true;
}