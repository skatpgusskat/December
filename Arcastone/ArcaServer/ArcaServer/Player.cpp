﻿#include "stdafx.h"
#include "Player.h"
#include "ArcaStone.h"


Player::Player()
{
	m_MaxCost = MAX_TURN;
	m_CurrentCost = MAX_TURN;
}


Player::~Player()
{
}


void Player::IsNearArca(std::vector<Unit*>* unitList, TurnManager* turnmanager)
{
	// 옆에 알카스톤이 있는지 보고, 전에 없었는데 이번에 있으면 이동가능횟수++
	// 반대면 --

	// 첫 턴이면 알카스톤적용X
	if (turnmanager->GetIsFirstTurn())
		return;

	if (!USE_ARCA)
		return;

	bool isNearArcaStoneNow = false;
	bool isNearArcaStoneBefore = false;
	ArcaStone* arcaStone = nullptr;

	// 아르카스톤 찾고
	for (auto unit : *unitList)
	{
		if (unit->GetUnitType() == UT_ARCASTONE)
		{
			arcaStone = dynamic_cast<ArcaStone*>(unit);
			break;
		}
	}

	// 아르카스톤 없는데여?
	if (arcaStone == nullptr)
		return;


	for (auto unit : m_UnitList)
	{
		//내 유닛이 아르카 스톤 옆에 있었습니까?
		if (unit.GetIsNearArca())
		{
			isNearArcaStoneBefore = true;
		}
	}

	for (auto unit : m_UnitList)
	{
		//내 유닛이 아르카 스톤 옆에 있습니까?
		for (int i = 1; i <= 6; i++) // Itor for HexaDirection
		{
			Coord positionGap = Coord(arcaStone->GetPos().x - unit.GetPos().x, arcaStone->GetPos().y - unit.GetPos().y);
			if (positionGap == GetUnitVector((HexaDirection)i))
			{
				isNearArcaStoneNow = true;
				unit.SetIsNearArca(true);
			}
			unit.SetIsNearArca(false);
		}
	}

	if (isNearArcaStoneNow)	// 이번 턴에 아르카스톤이 옆에 있고
	{
		if (isNearArcaStoneBefore)	// 전 턴에도 옆에 있었다면
		{
			// 알게뭐람
		}
		else				// 전 턴에는 옆에 없었다면
		{
			// 최대 이동가능 횟수를 1 증가시킨다.
			m_MaxCost++;
			// 현재 이동가능 횟수도 1 증가시킨다.
			m_CurrentCost++;
		}
	}
	else					// 이번 턴에 아르카스톤이 옆에 없고
	{
		if (isNearArcaStoneBefore)	// 전 턴에는 옆에 있었다면
		{
			m_MaxCost--;
			m_CurrentCost--;
		}
		else				// 전 턴에도 옆에 없었다면
		{
			// 무시
		}
	}
}

void Player::SetCurrentCost(int cost)
{
	if (cost > m_MaxCost)
		m_CurrentCost = m_MaxCost;
	else
		m_CurrentCost = cost;
}