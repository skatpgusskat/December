#include "GameScene.h"
#include "base/CCEventKeyboard.h"
#include "TcpClient.h"

#include "Header.h"
#include "Game.h"

Scene* GameScene::createScene()
{
    auto scene = Scene::create();
    
    auto layer = GameScene::create();

    // GameScene layer �� scene �� �߰�
    scene->addChild(layer, 1, string("base_layer"));

    return scene;
}

bool GameScene::init()
{
    if ( !LayerColor::initWithColor(ccc4(255,255,255,32)))
    {
        return false;
    }
    
    Size visibleSize = Director::getInstance()->getVisibleSize();
	Vec2 origin = Director::getInstance()->getVisibleOrigin();

	m_HWaitGameStart = CreateEvent(NULL, TRUE, FALSE, NULL);

	touchEventInit();

	drawHexagon();

	TcpClient::getInstance()->loginRequest();
	// ������ �α��� �ϰ� ����� !!

	// TODO : ������ ���� �������� �˷���� !!

	//WaitForSingleObject(m_HWaitGameStart, INFINITE);
	// �������� GameStartResult �� ���������� ������ �����

	Field* pField = Game::getInstance().getField();
	for (int i = 0; i < m_UnitDataLength; ++i)
	{
		pField->setUnitData(m_UnitData[i]);
	}
    


	this->schedule(schedule_selector(GameScene::gameLogic), 0.0f);

    return true;
}

void GameScene::gameLogic(float dt)
{
	// ������ �������� Ȯ���ϰ� �� ���ʶ�� 


}

void GameScene::touchEventInit()
{
	m_PointPathIndex = 0;
	m_IsClicked = false;

	_touchListener = EventListenerTouchOneByOne::create();
	_touchListener->setSwallowTouches(true);

	_touchListener->onTouchBegan = CC_CALLBACK_2(GameScene::onTouchBegan, this);

	EventDispatcher* dispatcher = Director::getInstance()->getEventDispatcher();

	dispatcher->addEventListenerWithSceneGraphPriority(_touchListener, this);
}

bool GameScene::onTouchBegan(Touch* touch, Event* event)
{
	while (m_PointPath.size() > 0)
		m_PointPath.pop_back();						// �� ����

	Point clickPoint = touch->getLocation();
	m_PointPath.push_back(clickPoint);				// ���� ����

	for (int i = 0; i < m_HexagonPoint.size(); ++i)
	{
		Point point = pointConversion(m_HexagonPoint.at(i));	// i ��° �ε����� ȭ��� ��ġ�� point�� ����

		if (touchCheck(clickPoint, point))
		{
			LabelTTF* vLabelx;
			vLabelx = LabelTTF::create("CLICK!", "Hevetica", 20);
			vLabelx->setPosition(Point(point.x, point.y));
			vLabelx->setColor(Color3B(0, 255, 0));

			this->addChild(vLabelx);
		}
	}

	m_IsClicked = true;

	return true;
}

void GameScene::onTouchMoved(Touch* touch, Event* event)
{
	Point clickPoint = touch->getLocation();

	for (int i = 0; i < m_HexagonPoint.size(); ++i)
	{
		Point point = pointConversion(m_HexagonPoint.at(i));	// i ��° �ε����� ȭ��� ��ġ�� point�� ����

		if (touchCheck(clickPoint, point))
		{
			LabelTTF* vLabelx;
			vLabelx = LabelTTF::create("DRAG!", "Hevetica", 20);
			vLabelx->setPosition(Point(point.x, point.y));
			vLabelx->setColor(Color3B(0, 255, 0));

			this->addChild(vLabelx);
		}
	}
}

bool GameScene::touchCheck(Point touch, Point anchor)
{
	// Ŭ���� ��ǥ�� Ư�� ������ �ȿ� ����ִ��� Ȯ���ϴ� �Լ�
	float msin = sin(RADIANS_60);

	if (touch.y > anchor.y - HEXAGON_LENGTH * msin &&
		touch.y < anchor.y + HEXAGON_LENGTH * msin)
	{
		// Ŭ���� ��ǥ�� �������� x ������ ���ϴ��� Ȯ��

		float yHowFarToAnchor = abs(touch.y - anchor.y);
		// yHowFarToAnchor : �������� �ܽɿ��� Ŭ���� ��ǥ�� y �������� ������ ����

		float yTuningValue = yHowFarToAnchor / (HEXAGON_LENGTH * msin) * 0.5;
		// �������� �߽ɿ��� y �Ÿ��� �־������� x ������ �������� ���� �����ϱ� ���ؼ� yTuningValue �� ������� .
		
		if (touch.x > anchor.x - HEXAGON_LENGTH * (1 - yTuningValue) &&
			touch.x < anchor.x + HEXAGON_LENGTH * (1 - yTuningValue))
			return true;
	}
	return false;
}

void GameScene::onTouchEnded(Touch* touch, Event* event)
{
	m_PointPathIndex = 0;

	m_IsClicked = false;
}

void GameScene::drawHexagon()
{
	CCDrawNode* node = CCDrawNode::create();
	this->addChild(node);

	Point point;

	for (int i = 0; i < MAP_SIZEX; ++i)
	{
		for (int j = 0; j < MAP_SIZEY; ++j)
		{
			if (drawToHexa(i, j) && MAP_IS_HEXA)	// ���������� �׸��� ���� ���ǹ�
				continue;

			point = pointConversion(Point(i, j));

			if (drawToRect(point.y) && MAP_IS_RECT)	// �簢������ �׸��� ���� ���ǹ�
				continue;

			m_HexagonPoint.push_back(Point(i, j));	// m_HexagonPoint �� ȭ�鿡 �׷����� ��ǥ(0~x, 0~y)���� ����

			Hexagon* hexa = createHexagon(point, HEXAGON_LENGTH);
			node->drawPolygon(&hexa->vertex[0], 6, ccc4f(0.0f, 0.0f, 0.0f, 0.0f), 1, ccc4f(0.2f, 1.0f, 0.2f, 0.3f));

			if(DRAW_HEXA_NUMBER) drawText(i, j, hexa);	// ����� �ȿ� ������ �ε��� ���� ������ ���ΰ�?
		}
	}
}

void GameScene::drawText(int i, int j, Hexagon* hexa)
{
	// ������ �ȿ� ������ �ε��� ���� �����ִ� �Լ� .
	// DRAW_HEXA_POSITION �� true�� �ε��� ���� �ƴ� ��ġ ���� ������
	int f = i;
	if (DRAW_HEXA_POSITION) f = hexa->anchor.x;
	char szBuf1[8];
	itoa(f, szBuf1, 10);

	f = j;
	if (DRAW_HEXA_POSITION) f = hexa->anchor.y;
	char szBuf2[8];
	itoa(f, szBuf2, 10);

	LabelTTF* vLabelx;
	LabelTTF* vLabely;
	vLabelx = LabelTTF::create(szBuf1, "Hevetica", 12);
	vLabely = LabelTTF::create(szBuf2, "Hevetica", 12);

	vLabelx->setPosition(Point(hexa->anchor.x - 7, hexa->anchor.y + 5));
	vLabely->setPosition(Point(hexa->anchor.x + 7, hexa->anchor.y - 5));

	vLabelx->setColor(Color3B(255, 255, 255));
	vLabely->setColor(Color3B(255, 0, 0));

	this->addChild(vLabelx);
	this->addChild(vLabely);
}

Point GameScene::pointConversion(Point point)
{
	// �Է��� index ������ point ���� ȭ�鿡 �׸� �� �ִ� point ������ ��ȯ���ִ� �Լ� .
	Point retPoint;

	retPoint.x = MAP_XSTART + HEXAGON_LENGTH * 1.5 * (point.x - (MAP_SIZEX - 1)*0.5);
	retPoint.y = MAP_YSTART - HEXAGON_LENGTH * sin(RADIANS_60) * (point.y * 2 - MAP_SIZEY + point.x - (MAP_SIZEX - 3)*0.5);
	// MAP_START �� �߾ӿ� ��ġ�ϵ��� �׷��ֱ� ���� ���ĵ� .

	return retPoint;
}

bool GameScene::drawToRect(float y)	
{
	// �簢���� �ǵ��� �׷��ִ� �κ��� �����ϱ� ���� �Լ� .
	if (y <= MAP_YSTART - HEXAGON_LENGTH * sin(RADIANS_60) * (MAP_SIZEY - MAP_SIZEX*0.5 + 1) ||
		y >= MAP_YSTART + HEXAGON_LENGTH * sin(RADIANS_60) * (MAP_SIZEY - MAP_SIZEX*0.5 + 1))
		return true;

	return false;
}

bool GameScene::drawToHexa(int x, int y)
{
	// �������� �ǵ��� �׷��ִ� �κ��� �����ϱ� ���� �Լ� .
	if (x + y >= MAP_SIZEX / 2 &&
		x + y <= MAP_SIZEX / 2 + MAP_SIZEY - 1)
		return false;

	return true;
}

Hexagon* GameScene::createHexagon(Point anchor, int size)
{
	// �߽����� ũ�⸦ ������, �׿� ���� �������� �� �������� ���Ϳ� �־ �����ϴ� �Լ� .
	Hexagon* newHexa = new Hexagon;
	newHexa->anchor = anchor;

	Point pos;
	float param_rad, param_cos, param_sin;
	for (int i = 0; i < 6; ++i)
	{
		param_rad = CC_DEGREES_TO_RADIANS(360 / 6 * i);
		param_cos = cos(param_rad);
		param_sin = sin(param_rad);

		pos.x = anchor.x + (size * param_cos);
		pos.y = anchor.y + (size * param_sin);
		newHexa->vertex.push_back(pos);
	}
	return newHexa;
}

void GameScene::ReadUnitData(UnitData* unitData, int length)
{
	m_UnitDataLength = length;

	for (int i = 0; i < length; ++i)
	{
		m_UnitData[i] = unitData[i];
	}

	SetEvent(m_HWaitGameStart);
}

void GameScene::drawUnit()
{
	for (int i = 0; i < MAX_UNIT_ON_GAME; ++i)
	{
		m_UnitSprite[i] = Sprite::create("WhitePawn.png");
	}
}