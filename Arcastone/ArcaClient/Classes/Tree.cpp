#include "Tree.h"


Tree::Tree()
{
}


Tree::~Tree()
{
}


void Tree::initSprite()
{
	Unit::initSprite();
	assert(m_Owner == UO_NPC);

	Sprite* unitSprite;
	unitSprite = Sprite::create("npc_tree.png");

	unitSprite->setScale(HEXAGON_LENGTH*1.5 / unitSprite->getContentSize().width);
	unitSprite->setAnchorPoint(Vec2(0.5f, 0.3f));

	m_Sprite->addChild(unitSprite);
}