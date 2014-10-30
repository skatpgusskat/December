#include "HelloWorldScene.h"
#include "base/CCEventKeyboard.h"
#include "TcpClient.h"

USING_NS_CC;

Scene* HelloWorld::createScene()
{
    // 'scene' is an autorelease object
    auto scene = Scene::create();
    
    // 'layer' is an autorelease object
    auto layer = HelloWorld::create();

    // add layer as a child to scene
    scene->addChild(layer, 1, std::string("base_layer"));

    // return the scene
    return scene;
}

// on "init" you need to initialize your instance
bool HelloWorld::init()
{
    //////////////////////////////
    // 1. super init first
    if ( !LayerColor::initWithColor(ccc4(255,255,255,255)))
    {
        return false;
    }
    
    Size visibleSize = Director::getInstance()->getVisibleSize();
    Vec2 origin = Director::getInstance()->getVisibleOrigin();



	TcpClient::getInstance()->loginRequest();
    
	this->schedule(schedule_selector(HelloWorld::gameLogic), 1.0f);

	this->setKeyboardEnabled(true);

    return true;
}


void HelloWorld::spriteMoveFinished(CCNode* sender)
{
	CCSprite *sprite = (CCSprite *)sender;
	this->removeChild(sprite, true);
	
}

void HelloWorld::gameLogic(float dt)
{
	
}

void HelloWorld::onKeyReleased(EventKeyboard::KeyCode keyCode, Event* event)
{
	static int id = 1;

	switch (keyCode)
	{
	case cocos2d::EventKeyboard::KeyCode::KEY_UP_ARROW:
		TcpClient::getInstance()->moveRequest(mPlayerSprite->getPosition().x, mPlayerSprite->getPosition().y + 10);
		break;
	case cocos2d::EventKeyboard::KeyCode::KEY_DOWN_ARROW:
		TcpClient::getInstance()->moveRequest(mPlayerSprite->getPosition().x, mPlayerSprite->getPosition().y - 10);
		break;
	case cocos2d::EventKeyboard::KeyCode::KEY_LEFT_ARROW:
		TcpClient::getInstance()->moveRequest(mPlayerSprite->getPosition().x - 10, mPlayerSprite->getPosition().y);
		break;
	case cocos2d::EventKeyboard::KeyCode::KEY_RIGHT_ARROW:
		TcpClient::getInstance()->moveRequest(mPlayerSprite->getPosition().x + 10, mPlayerSprite->getPosition().y);
		break;

	case cocos2d::EventKeyboard::KeyCode::KEY_L: ///< LOGIN
		TcpClient::getInstance()->loginRequest();
		break;

	case cocos2d::EventKeyboard::KeyCode::KEY_C: ///< CHAT
		TcpClient::getInstance()->chatRequest(" - HELLO~~ THIS IS TEST...");
		break;


	default:
		break;
	}
	
	
}

void HelloWorld::updateMe(float x, float y)
{
	mPlayerSprite->setPosition(x, y);
}

void HelloWorld::updatePeer(int id, float x, float y)
{
	auto it = mPeerMap.find(id);
	if (it == mPeerMap.end())
	{
		CCSprite* newPeer = CCSprite::create("Target.png", CCRectMake(0, 0, 27, 40));
		newPeer->setPositionX(x);
		newPeer->setPositionY(y);
		this->addChild(newPeer);
		mPeerMap.insert(std::map<int, cocos2d::CCSprite*>::value_type(id, newPeer));
	}
	else
	{
		it->second->setPositionX(x);
		it->second->setPositionY(y);
	}

}

void HelloWorld::chatDraw(const std::string& from, const std::string& chat)
{

	CCLOG("CHAT FROM ID[%s] Chat[%s]", from.c_str(), chat.c_str());
}