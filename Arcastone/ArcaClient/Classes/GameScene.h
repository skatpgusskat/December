#ifndef __HELLOWORLD_SCENE_H__
#define __HELLOWORLD_SCENE_H__

#include "Header.h"
#include "Game.h"
#include "Unit.h"

enum GameState{
	GS_BEFORE_LOGIN,
	GS_WAIT_LOGIN,
	GS_GAME_START,
};
class GameScene : public LayerColor
{
public:
    // there's no 'id' in cpp, so we recommend returning the class instance pointer
    static Scene*			createScene();

    // Here's a difference. Method 'init' in cocos2d-x returns bool, instead of returning 'id' in cocos2d-iphone
    virtual bool			init();
    
    // a selector callback
    void					menuCloseCallback(Ref* pSender);

	void					gameLogic(float dt);

	EventListenerTouchOneByOne*	_touchListener;
	void					touchEventInit();
	virtual bool			onTouchBegan(Touch* touch, Event* event);
	virtual void			onTouchMoved(Touch* touch, Event* unused_event);
	virtual void			onTouchCancelled(Touch* touch, Event* unused_event){}
	virtual void			onTouchEnded(Touch* touch, Event *unused_event);
	bool					touchCheck(Point touch, Point anchor);

	CREATE_FUNC(GameScene);

	Hexagon*				createHexagon(Point anchor, int size);
	void					drawHexagon();
	void					drawUnit();
	void					drawText(int i, int j, Hexagon* hexa);

	Point					conversionIndexToPoint(Point point);
	Point					conversionPointToIndex(Point point);
	bool					drawToRect(float y);
	bool					drawToHexa(int x, int y);

	float					getPointToPointDirection(Point point1, Point point2);
	float					getPointToPointDistance(Point point1, Point point2);

	// 하위는 네트워크 관련 함수
	void					ReadUnitData(UnitData unitData[], int length);
	// Header 안에서 typedef Packet::GameStartResult::UnitData UnitData 했음 .
	void					SetTurn(bool isMyTurn){ m_IsMyTurn = isMyTurn; }

private:
	Game					m_Game;
	GameState				m_GameState;
	bool					m_IsMyTurn;
	bool					m_IsAction;

	Player					m_Player[2];

	vector<Point>			m_PathPoint;
	int						m_PathPointIndex;
	vector<CCDrawNode*>		m_PathDrawNode;
	int						m_SelectedUnitIndex;

	vector<Point>			m_HexagonPoint;
	CCSprite*				m_UnitSprite[MAX_UNIT_ON_GAME];
	CCDrawNode*				m_UnitDrawNode[MAX_UNIT_ON_GAME];	// 유닛 그리는 실험을 위한 임시 변수
};

#endif // __HELLOWORLD_SCENE_H__
