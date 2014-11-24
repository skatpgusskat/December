﻿#include "stdafx.h"
#include "Field.h"


Field::Field()
{

}


Field::~Field()
{	
}

void Field::InitField(int fieldWidth, int fieldHeight)
{
	// TODO : 맵의 형태 불러와서 그거대로 FieldBlock 찍어내기
	// 일단은 육각형 형태로 잘라냄..
	
	for (int x = 0; x < MAP_FIELD_WIDTH; ++x)
	{
		for (int y = 0; y < MAP_FIELD_HEIGHT; ++y)
		{
			if (x + y >= MAP_FIELD_WIDTH / 2 &&
				x + y <= MAP_FIELD_WIDTH / 2 + MAP_FIELD_HEIGHT - 1)
			{
				FieldBlock newFieldBlock;
				Coord Pos(x, y);
				newFieldBlock.m_Position = Pos;
				newFieldBlock.m_Status = FBS_NONE;
				newFieldBlock.m_Type = FBT_NORMAL;

				m_FieldBlockList.insert(FieldBlockList::value_type(Pos, newFieldBlock));
			}
		}
	}

	m_FieldSizeX = MAP_FIELD_WIDTH;
	m_FieldSizeY = MAP_FIELD_HEIGHT;
}

bool Field::IsInsideOfField(Coord position)
{
	FieldBlockList::iterator it = m_FieldBlockList.find(position);
	// 해당 블록이 존재하고
	if (it != m_FieldBlockList.end())
	{
		// 구멍나지 않았으면
		if (it->second.m_Type != FBT_HOLE)
		{
			return true; // 생존!
		}
	}
	return false;
}

void Field::SetFieldType(Coord fieldCoord, FieldBlockType fieldType)
{
	FieldBlockList::iterator it = m_FieldBlockList.find(fieldCoord);
	it->second.m_Type = fieldType;
}

void Field::GetFieldBlockList(OUT FieldBlock fieldBlockArr[])
{
	int i = 0;
	for (auto field : m_FieldBlockList)
	{
		fieldBlockArr[i] = field.second;
		++i;
	}
}
