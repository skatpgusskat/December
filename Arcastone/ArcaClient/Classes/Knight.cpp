﻿#include "Knight.h"

Knight::Knight()
{
}

Knight::~Knight()
{
}

void Knight::initSprite()
{
	Unit::initSprite();
	assert(m_Owner != UO_NONE && "unknown unit owner");

	Sprite* unitSprite;
	unitSprite = Sprite::create("unit_magician.png");

	unitSprite->setScale(HEXAGON_LENGTH*1.5 / unitSprite->getContentSize().width);
	unitSprite->setAnchorPoint(Vec2(0.5f, 0.3f));

	m_Sprite->addChild(unitSprite);
}