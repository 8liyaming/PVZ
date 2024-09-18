/*
	开发日志：2024/07/25始
	1.创建新项目
	3.实现游戏场景
	4.实现游戏顶部的卡槽
	5.实现卡槽中的植物卡牌
	6.实现植物的种植
	7.实现植物的摇摆动作
	8.实现阳光
	9.实现发射子弹
	10.实现僵尸死亡
	11.实现僵尸吃植物
	12.实现坚果墙
	13.实现铲子
	14.实现游戏暂停页面
	15.实现游戏重开
	16.实现回到主菜单并再次回到游戏
	17.实现土豆地雷
	18.实现卡牌冷却
	19.实现路障和铁桶僵尸
	20.实现多个僵尸一起出场
	2023/08/27日阶段性结束
*/

#include <stdio.h>
#include <graphics.h>						//easyx图形库的头文件
#include "tools.h"							//图片透明度改变的代码
#include <time.h>
#include <mmsystem.h>						//播放背景音乐和音效的头文件
#pragma comment(lib, "winmm.lib")			//播放音乐需要的库文件

#define WIN_Width 900						//游戏窗口大小
#define WIN_Height 600
#define curX00 136							//第一行第一列草坪块的左上角位置的x值（减去120）（原256）
#define curY00 95							//第一行第一列草坪块的左上角位置的y值
#define cur_Height 100						//每一个草坪块的x的长度
#define cur_Width 81					    //每一个草坪块的y的长度
#define zm_BeforeWave 10					//一大波之前一共会出现多少个僵尸

enum { PeaShooter, SunFlower, WallNut, PotatoMine, Cards };		//可以保证在无论添加多少种植物，Cards的整型值都会更新为植物种类数
enum { WallNut_Normal, WallNut_Cracked, WallNut_Cracked2, PotatoMine_Under, PotatoMine_Ready, Plants_Status };	//坚果墙和土豆地雷的状态
enum { START, GOING, PAUSE, WIN, FAIL };			//游戏的状态，用于判断胜利或者结束
enum { NormalZM, RoadConeZM, BucketZM, ZMType };	//僵尸的种类

IMAGE imgBg;			//表示背景图片
IMAGE imgBar;			//表示植物卡槽
IMAGE imgCards[Cards], imgCardsBlack[Cards], imgCardsCD[Cards];	//表示植物卡牌和黑白卡牌（阳光不够购买）和冷却中的卡牌
IMAGE* imgPlants[Cards][20];				//用数组保存指针IMAGE变量，表示植物的每一帧图片
IMAGE* imgPlantsStatus[Plants_Status][17];	//植物独特状态的图片（坚果墙时就是普通和损坏三种状态，土豆地雷就是在底下和出来两种状态
IMAGE imgSunBall[29];						//阳光的每一帧图片
IMAGE* imgZM[ZMType][22];					//僵尸走路和吃植物的每一帧图片（普通和路障和铁桶僵尸）
IMAGE* imgZMEat[ZMType][21];
IMAGE imgBulletNormal, imgBulletBlast[4];	//豌豆子弹正常状态图片（未爆炸）以及爆炸状态图片
IMAGE imgZMDead[10];						//僵尸死亡状态图片以及僵尸吃植物状态图片
IMAGE imgShovel, imgShovelSlot;				//铲子图片以及放铲子的框的图片
IMAGE imgFail, imgWin;						//输掉和胜利的图片
IMAGE imgPause, imgPauseMenu;				//右上角暂停的图片以及暂停菜单的图片
IMAGE imgBg2, imgMenu1, imgMenu2, imgMenuQuit1, imgMenuQuit2;	//最开始游戏菜单
IMAGE imgReady, imgSet, imgPlant;			//准备安放植物的三张图片
IMAGE imgboom[2];							//两张土豆地雷爆炸的图片

struct PLANTS {				//结构体名字又用不上，而且为了防止写代码时候错写成结构体名字，我就用拼音表示了
	int type;				//植物种类，0：没有植物 1：第一种植物
	int frameIndex;			//序列帧序号
	bool catched;			//是否正在被僵尸吃（这个catched的意思应该是是否被吃过，因为会有僵尸吃到一半然后就被打死了，这时候deadTime并不能清0，这个catched应该可以辅助判断）
	int deadTime;			//植物血量
	int x, y;				//植物二维坐标
	int row, col;			//植物的行列值
	int status;				//植物状态（坚果墙时就是普通和损坏三种状态，土豆地雷就是在底下和出来两种状态)
	bool shovelAndCatched;	//是否在被僵尸吃的同时被铲子铲掉
	int prepareTime;		//准备时间（土豆地雷)
	bool boom;				//是否爆炸（土豆地雷）
}map[5][9];

struct SUN {
	float x, y;			//阳光飘落过程实时坐标
	int frameIndex;		//阳光序列帧序号
	int destY;			//飘落目标位置的y坐标
	bool used;			//是否在被使用
	int timer;			//阳光落地后的计时
	float k;			//点击位置和终点位置相连的线的斜率
	bool click;			//是否被点击
	bool flower;		//是否由向日葵生成
}balls[15];				//用到“池”的概念 

struct ZOMBIES {
	int type;			//僵尸种类
	int x, y;			//僵尸的实时坐标
	int frameIndex;		//僵尸序列帧序号
	bool used;			//是否被使用
	int speed;			//每次移动位移（速度）
	int row;			//所在行（便于豌豆子弹的设置）
	int blood;			//僵尸血量
	bool dead;			//是否死亡
	bool eating;		//是否正在吃植物
}zms[zm_BeforeWave];

struct BULLETS {			//豌豆子弹的结构体
	int x, y;
	int row;
	bool used;
	int speed;
	bool blast;				//是否发生爆炸
	int framesIndex;		//爆炸的帧序号
}bullets[30];

struct JUDGECARDS {
	int type;			//种类
	bool costable;		//是否阳光数足够购买（这里的两个able的设定看那个drawCards函数）
	bool CDable;		//是否冷却完毕
	int cost;			//购买所需阳光
	int CD;				//冷却时间
}cards[Cards];

int ballMax = sizeof(balls) / sizeof(balls[0]);			//“池”中所有阳光球的数量
int zmMAX = sizeof(zms) / sizeof(zms[0]);				//“池”中所有僵尸的数量
int bulletMax = sizeof(bullets) / sizeof(bullets[0]);	//“池”中所有豌豆子弹的数量
int curX, curY;				//当前选中的植物，在移动过程中的二维坐标位置
int curPlants;				//植物种类，0:没有选中, 1:选择第一种植物
int sunSum;					//阳光数量，初始为50
bool judgeShovel;			//判断铲子是否捡起来
bool judgePlants;			//判断植物是否捡起来
bool restart = false;		//判断游戏是否重开
bool backToMenu = false;	//判断游戏是否回到主页面
bool musicOver = false;		//判断开场音效是否播放完毕
int gameStatus = START;		//游戏目前状态（胜利或者结束）
int killCount;				//击杀僵尸的计数
int zmCount;				//已经出现的僵尸的计数

//函数声明部分
void gameInit();						// 加载游戏资源（只运行一次）

bool fileExist(const char* name);				//判断文件是否存在
void gameLoad();								//游戏初始化

void startUI();								//显示游戏主界面（包含以下函数）
int clickStart(ExMessage* msg, int flag1);		//点击开始游戏
int clickQuit(ExMessage* msg, int flag2);		//点击退出游戏

void readySetPlant();

void userClick();								//鼠标点击（包含以下函数）
void collectSun(ExMessage* msg);				//捡阳光球
void growPlants(ExMessage* msg);				//鼠标种植植物
void clickShovel(ExMessage* msg);				//铲子铲掉植物
void clickPause(ExMessage* msg);				//暂停菜单

void checkGamePause();		//游戏暂停页面

void updateGame();			//持续更新游戏画面（包含以下函数）
void judgeCards();			//判断阳光数是否足以购买植物
void updatePlants();		//更新植物状态
void createFlowerSun();		//创建向日葵的阳光
void createSkySun();		//创建天上的阳光
void updateSun();			//更新阳光状态
void createZM();			//创建僵尸
void updateZM();			//更新僵尸状态
void createShoot();			//创建豌豆子弹
void updateshoot();			//更新豌豆子弹，课程视频里叫做updateBullets
void checkBullets2ZM();		//子弹和僵尸的碰撞检测以及僵尸死亡检测
void checkPotatoMine2ZM();	//土豆地雷对僵尸的爆炸检测
void plantsDie(int row, int k);//判断植物的死亡是被铲掉还是被吃掉，并更新草坪块和僵尸的状态
void checkZM2Plants();		//僵尸吃植物的碰撞检测

void updateDraw();			//更新（显示/渲染）游戏窗口（包含以下函数）
void drawCards();			//渲染卡槽中的植物
void drawPlants();			//渲染拖动过程中的和种下的植物
void drawShovel();			//渲染铲子
void drawSunBalls();		//根据帧数渲染阳光球图片
void drawZM();				//根据帧数渲染僵尸图片
void drawBullets();			//渲染豌豆子弹
void drawSunNumber();		//输出字体（阳光数字）

void checkGameOver();		//检查游戏是否结束（胜利或者失败）

int main() {
	gameInit();					//加载游戏资源（只运行一次）
	gameLoad();					//游戏初始化（可以用于重开游戏时再次初始化）

	int timer = 0;
	bool flag = true;			//设置延迟
	while (1) {
		startUI();				//显示游戏主界面
		readySetPlant();
		userClick();			//鼠标点击
		checkGamePause();		//暂停菜单

		timer += getDelay();    //设置帧等待（延时）（小技巧）
		if (timer > 20) {
			flag = true;
			timer = 0;
		}
		if (flag) {
			updateGame();			//持续更新游戏画面（植物摇摆等）
			updateDraw();			//更新（显示/渲染）游戏窗口
			checkGameOver();		//检测游戏状态，赢或输
			flag = false;
		}
	}
	system("pause");
	return 0;
}

void gameInit() {			//加载游戏资源（只运行一次）
	loadimage(&imgBg2, "res/menu.png");						//背景图片
	loadimage(&imgMenu1, "res/menu1.png");					//原先开始游戏图片
	loadimage(&imgMenu2, "res/menu2.png");					//高亮开始游戏图片
	loadimage(&imgMenuQuit1, "res/menu_quit1.png");			//开始界面的退出游戏
	loadimage(&imgMenuQuit2, "res/menu_quit2.png");			//开始界面的退出游戏高亮图片
	loadimage(&imgBg, "res/map0.jpg");						//加载背景图片，记得把字符集改为“多字节字符集”
	loadimage(&imgBar, "res/bar5.png");						//卡槽图片
	loadimage(&imgBulletNormal, "res/bullet_normal.png");	//豌豆子弹图片
	loadimage(&imgShovel, "res/shovel.png");				//铲子图片
	loadimage(&imgShovelSlot, "res/shovelSlot.png");		//放铲子的框的图片
	loadimage(&imgFail, "res/fail.png");					//输掉的图片
	loadimage(&imgWin, "res/win.png");						//胜利的图片
	loadimage(&imgPause, "res/pause.png");					//暂停的图片
	loadimage(&imgPauseMenu, "res/pauseMenu.png");			//暂停菜单的图片
	loadimage(&imgReady, "res/ready.png");					//准备安放植物的三张图片
	loadimage(&imgSet, "res/set.png");
	loadimage(&imgPlant, "res/plants.png");
	loadimage(&imgboom[0], "res/zhiwu/status/boom/boom1.png");	//土豆地雷爆炸后的两张图片
	loadimage(&imgboom[1], "res/zhiwu/status/boom/boom2.png");

	memset(imgPlants, 0, sizeof(imgPlants));				//初始化数组函数，即将数组中所有元素置为0
	memset(imgPlantsStatus, 0, sizeof(imgPlantsStatus));	//植物特殊状态初始化
	memset(imgZM, 0, sizeof(imgZM));
	memset(imgZMEat, 0, sizeof(imgZMEat));
	memset(map, 0, sizeof(map));							//草坪块数组初始化
	memset(balls, 0, sizeof(balls));						//阳光球数组初始化
	memset(zms, 0, sizeof(zms));							//僵尸结构体数组初始化
	memset(bullets, 0, sizeof(bullets));					//子弹结构体数组初始化
	memset(cards, 0, sizeof(cards));						//卡槽植物状态初始化

	//初始化植物卡牌
	char name[64];
	for (int i = 0; i < Cards; i++) {		//生成植物卡牌的文件名
		sprintf_s(name, sizeof(name), "res/Cards/card_%d.png", i + 1);			//加载正常卡牌
		loadimage(&imgCards[i], name);
		sprintf_s(name, sizeof(name), "res/Cards_Black/card_%d.png", i + 1);	//加载黑白卡牌（阳光数不够购买）
		loadimage(&imgCardsBlack[i], name);
		sprintf_s(name, sizeof(name), "res/Cards_CD/card_%d.png", i + 1);		//加载冷却中的卡牌
		loadimage(&imgCardsCD[i], name);

		for (int j = 0; j < 20; j++) {											//因为目前植物的动态图片最多为向日葵的18张
			sprintf_s(name, sizeof(name), "res/Plants/%d/%d.png", i, j + 1);		//同上
			//先判断这个文件是否存在（意为有的植物的动态图片不够20张，那么就只加载存在的植物图片，不存在的不加载
			if (fileExist(name)) {  //简单粗暴方法，直接打开文件，能打开肯定存在
				imgPlants[i][j] = new IMAGE;		//分配内存,如果没有这个，那么下面的loadimage时imgPlants里面全是空指针，无法实现，程序会崩溃。
				loadimage(imgPlants[i][j], name);
			}
			else
				break;
		}
	}
	for (int i = 0; i < Plants_Status; i++) {		//加载坚果墙和土豆地雷的特殊状态
		for (int j = 0; j < 17; j++) {
			sprintf_s(name, sizeof(name), "res/Plants/status/%d/%d.png", i, j + 1);
			if (fileExist(name)) {
				imgPlantsStatus[i][j] = new IMAGE;
				loadimage(imgPlantsStatus[i][j], name);
			}
			else
				break;
		}
	}

	for (int i = 0; i < 29; i++) {				//加载阳光球的每一帧图片
		sprintf_s(name, sizeof(name), "res/sunshine/%d.png", i + 1);
		loadimage(&imgSunBall[i], name);
	}
	for (int i = 0; i < ZMType; i++) {				//加载三种僵尸正常走路的每一帧图片
		for (int j = 0; j < 22; j++) {
			sprintf_s(name, sizeof(name), "res/zm/%d/%d.png", i, j + 1);
			if (fileExist(name)) {
				imgZM[i][j] = new IMAGE;
				loadimage(imgZM[i][j], name);
			}
			else
				break;
		}
	}
	for (int i = 0; i < ZMType; i++) {				//加载三种僵尸吃植物的每一帧图片
		for (int j = 0; j < 22; j++) {
			sprintf_s(name, sizeof(name), "res/zm_eat/%d/%d.png", i, j + 1);
			if (fileExist(name)) {
				imgZMEat[i][j] = new IMAGE;
				loadimage(imgZMEat[i][j], name);
			}
			else
				break;
		}
	}
	//加载豌豆子弹爆炸的图片帧数组（一共四张，第四张为正常图片，前三张为正常图片缩小一定倍数后得到）
	loadimage(&imgBulletBlast[3], "res/bullet_blast.png");
	for (int i = 0; i < 3; i++) {
		float k = (i + 1) * 0.2;
		loadimage(&imgBulletBlast[i], "res/bullet_blast.png",
			imgBulletBlast[3].getwidth() * k,
			imgBulletBlast[3].getheight() * k, true);		//倍数缩小
	}
	for (int i = 0; i < 10; i++) {				//加载僵尸死亡时的每一帧图片
		sprintf_s(name, sizeof(name), "res/zm_dead/%d.png", i + 1);
		loadimage(&imgZMDead[i], name);
	}
	//配制随机种子
	srand(time(NULL));
	//创建游戏的图形窗口
	initgraph(WIN_Width, WIN_Height);
	//设置字体
	LOGFONT f;
	gettextstyle(&f);
	f.lfHeight = 30;
	f.lfWeight = 15;
	strcpy(f.lfFaceName, "Segoe UI Black");
	f.lfQuality = ANTIALIASED_QUALITY;				//抗锯齿效果（字体边缘）
	settextstyle(&f);
	setbkmode(TRANSPARENT);							//设置透明效果
	setcolor(BLACK);
}

void startUI() {		//显示游戏主界面
	if (backToMenu == true) {		//返回主菜单
		gameStatus = START;
		backToMenu = false;
		mciSendString("pause bg2", 0, 0, 0);
	}
	if (restart == true) {			//重开游戏
		gameLoad();					//重新加载一下
		mciSendString("close bg2", 0, 0, 0);
		gameStatus = GOING;
		return;
	}
	if (gameStatus == WIN || gameStatus == FAIL || gameStatus == GOING || (gameStatus == PAUSE && restart != true))
		return;					//如果没有重开游戏，那么以下的东西只加载一次
	if (gameStatus == START) {
		int flag1 = 1;
		int flag2 = 1;

		mciSendString("open res/audio/bg.mp3 alias bg", 0, 0, 0); 	//播放背景音乐并修改音量
		mciSendString("play bg repeat", 0, 0, 0);
		mciSendString("setaudio bg volume to 300", 0, 0, 0);

		while (1) {
			BeginBatchDraw();  //开始缓冲（防止不停抖动）
			putimage(0, 0, &imgBg2);
			putimagePNG(474, 75, flag1 ? &imgMenu1 : &imgMenu2);		//根据下面的判断输出原始图片还是高亮后的图片
			putimagePNG(810, 510, flag2 ? &imgMenuQuit1 : &imgMenuQuit2);		//根据下面的判断输出原始图片还是高亮后的图片（退出选项）
			ExMessage msg;
			if (peekmessage(&msg)) {
				flag1 = clickStart(&msg, flag1);		//点击开始的地方
				flag2 = clickQuit(&msg, flag2);			//点击退出的地方
				if (flag1 == 3) {
					gameStatus = GOING;
					mciSendString("play bg2 repeat", 0, 0, 0);		//在回到主界面后再次进入游戏会继续播放
					return;
				}
			}
			EndBatchDraw();  //结束缓冲
		}
	}
}

void gameLoad() {		//游戏初始化（可重复加载）

	memset(map, 0, sizeof(map));							//草坪块数组初始化
	memset(balls, 0, sizeof(balls));						//阳光球数组初始化
	memset(zms, 0, sizeof(zms));							//僵尸结构体数组初始化
	memset(bullets, 0, sizeof(bullets));					//子弹结构体数组初始化
	memset(cards, 0, sizeof(cards));						//卡槽植物状态初始化

	for (int i = 0; i < Cards; i++) {			//卡槽状态设置
		cards[i].type = i;
		if (cards[i].type == PeaShooter)
			cards[i].cost = 100;
		else if (cards[i].type == SunFlower || cards[i].type == WallNut)
			cards[i].cost = 50;
		else if (cards[i].type == PotatoMine)
			cards[i].cost = 25;
	}

	killCount = 0;		//初始化杀僵尸的计数
	zmCount = 0;
	sunSum = 50;
}

void readySetPlant() {			//准备放植物的三个图片
	static bool music = true;
	static int count = 0;
	if (restart == true) {
		music = true;
		count = 0;
		musicOver = false;
		restart = false;		//restart是在这里重置为false的
	}
	if (music == true) {
		mciSendString("play res/audio/readysetplant.mp3", 0, 0, 0);		//只播放一次音乐
		music = false;
	}
	while (1) {
		BeginBatchDraw();		//只要有while和输出图片的地方就一定要有双缓冲！！！
		if (musicOver == true)
			break;
		count++;

		putimage(-120, 0, &imgBg);				//输出图片（所有图片输出函数的坐标减去120）
		putimagePNG(50, 0, &imgBar);			//透明度改变函数（原250）减去200（就是卡槽部分全部减去200，其他部分全部减去120）
		putimagePNG(660, 0, &imgShovelSlot);	//放铲子的框
		putimagePNG(785, 0, &imgPause);			//右上角暂停选项
		drawCards();			//渲染卡槽中的植物
		drawSunNumber();		//输出字体（阳光数字）

		if (count < 400)
			putimagePNG(400, 250, &imgReady);
		else if (count > 400 && count < 700)
			putimagePNG(400, 250, &imgSet);
		else if (count > 700 && count < 1200)
			putimagePNG(380, 230, &imgPlant);
		else if (count > 1200) {
			count = 0;
			musicOver = true;
			mciSendString("open res/audio/grasswalk.mp3 alias bg2", 0, 0, 0);
			mciSendString("play bg2 repeat", 0, 0, 0);
			mciSendString("setaudio bg2 volume to 300", 0, 0, 0);
			break;
		}

		EndBatchDraw();
	}
}

void userClick() {						//鼠标点击
	ExMessage msg;  //消息类型
	if (peekmessage(&msg)) {     //peekmessage判断有没有消息，用getmessage直接读，当没有鼠标动作时会卡
		clickPause(&msg);		//暂停菜单
		collectSun(&msg);		//捡阳光
		growPlants(&msg);		//种下植物
		clickShovel(&msg);		//铲子产植物
	}
}

void checkGamePause() {			//游戏暂停菜单
	if (gameStatus == GOING)
		return;
	else {
		ExMessage msg;
		while (1) {
			BeginBatchDraw();
			if (gameStatus == PAUSE) {
				putimagePNG(250, 50, &imgPauseMenu);
				EndBatchDraw();
			}
			if (peekmessage(&msg)) {
				if (gameStatus == GOING)
					return;
				if (msg.message == WM_LBUTTONDOWN && msg.x > 250 + 50 && msg.x < 250 + 375 && msg.y > 50 + 400 && msg.y < 50 + 470 && gameStatus == PAUSE) {
					//鼠标左键按下
					mciSendString("play res/audio/buttonclick.mp3", 0, 0, 0);
				}
				else if (msg.message == WM_LBUTTONUP && msg.x > 250 + 50 && msg.x < 250 + 375 && msg.y > 50 + 400 && msg.y < 50 + 470 && gameStatus == PAUSE) {
					//鼠标左键抬起
					mciSendString("play bg2 repeat", 0, 0, 0);
					gameStatus = GOING;
				}
				else if (msg.message == WM_LBUTTONUP && msg.x > 250 + 95 && msg.x < 250 + 330 && msg.y > 50 + 280 && msg.y < 50 + 320 && gameStatus == PAUSE) {
					mciSendString("play res/audio/buttonclick.mp3", 0, 0, 0);
					restart = true;
					return;
				}
				else if (msg.message == WM_LBUTTONUP && msg.x > 250 + 95 && msg.x < 250 + 330 && msg.y > 50 + 330 && msg.y < 50 + 370 && gameStatus == PAUSE) {
					mciSendString("play res/audio/buttonclick.mp3", 0, 0, 0);
					backToMenu = true;
					return;
				}
			}
		}
	}
}

void updateGame() {		//持续更新游戏画面
	judgeCards();		//判断卡牌是否可以点击
	updatePlants();		//更新植物状态
	createFlowerSun();	//创建向日葵的阳光
	createSkySun();		//创建天上的阳光
	updateSun();		//更新阳光状态
	createZM();			//创建僵尸
	updateZM();			//更新僵尸状态
	createShoot();		//创建豌豆子弹
	updateshoot();		//更新豌豆子弹，课程视频里叫做updateBullets
	checkBullets2ZM();	//子弹和僵尸的碰撞检测以及僵尸死亡检测
	checkPotatoMine2ZM();//土豆地雷对僵尸的爆炸检测
	checkZM2Plants();	//僵尸吃植物的碰撞检测
}

void updateDraw() {	//更新（显示/渲染）游戏窗口
	BeginBatchDraw();						//开始缓冲（双缓冲）

	putimage(-120, 0, &imgBg);				//输出图片（所有图片输出函数的坐标减去120）
	putimagePNG(50, 0, &imgBar);			//透明度改变函数（原250）减去200（就是卡槽部分全部减去200，其他部分全部减去120）
	putimagePNG(660, 0, &imgShovelSlot);	//放铲子的框
	putimagePNG(785, 0, &imgPause);			//右上角暂停选项

	drawCards();			//渲染卡槽中的植物
	drawSunNumber();		//输出字体（阳光数字）
	drawPlants();			//渲染种下的植物和铲子
	drawShovel();			//渲染铲子
	drawSunBalls();			//根据帧数渲染阳光球图片
	drawZM();				//根据帧数渲染僵尸图片
	drawBullets();			//渲染豌豆子弹

	EndBatchDraw();	       //结束缓冲（双缓冲）
}

void checkGameOver() {
	if (gameStatus == FAIL) {
		putimagePNG(220, 80, &imgFail);
		mciSendString("close bg2", 0, 0, 0);
		mciSendString("play res/audio/lose.mp3", 0, 0, 0);
		MessageBox(NULL, "Game Over!", "over", 0);	//游戏结束（待优化）
		exit(0);
	}
	else if (gameStatus == WIN) {
		putimagePNG(300, 250, &imgWin);
		mciSendString("close bg2", 0, 0, 0);
		mciSendString("play res/audio/win.mp3", 0, 0, 0);
		MessageBox(NULL, "You are victorious!", "over", 0);
		exit(0);
	}
}

bool fileExist(const char* name) {			//判断文件是否存在
	FILE* fp = fopen(name, "r");
	if (fp == NULL) {
		return false;
	}
	else {
		fclose(fp);
		return true;
	}
}

int clickStart(ExMessage* msg, int flag1) {			//点击开始游戏
	if (msg->message == WM_MOUSEMOVE &&
		msg->x > 474 && msg->x < 474 + 331 &&
		msg->y > 75 && msg->y < 75 + 140) {
		return 0;
	}
	else if (!(msg->x > 474 && msg->x < 474 + 331 &&
		msg->y > 75 && msg->y < 75 + 140)) {
		return 1;
	}
	else if (msg->message == WM_LBUTTONDOWN &&
		msg->x > 474 && msg->x < 474 + 331 &&
		msg->y > 75 && msg->y < 75 + 140) {
		mciSendString("play res/audio/gravebutton.mp3", 0, 0, 0);
		return 2;
	}
	else if (msg->message == WM_LBUTTONUP && flag1 == 2) {
		mciSendString("close bg", 0, 0, 0);
		return 3;
	}
}

int clickQuit(ExMessage* msg, int flag2) {		//点击退出游戏
	if (msg->message == WM_MOUSEMOVE &&
		msg->x > 810 && msg->x < 810 + 47 &&
		msg->y > 510 && msg->y < 510 + 27) {
		return 0;
	}
	else if (!(msg->x > 810 && msg->x < 810 + 47 &&
		msg->y > 510 && msg->y < 510 + 27)) {
		return 1;
	}
	else if (msg->message == WM_LBUTTONDOWN &&
		msg->x > 810 && msg->x < 810 + 47 &&
		msg->y > 510 && msg->y < 510 + 27) {
		mciSendString("play res/audio/gravebutton.mp3", 0, 0, 0);
		return 2;
	}
	else if (msg->message == WM_LBUTTONUP && flag2 == 2) {
		exit(0);
	}
}

void collectSun(ExMessage* msg) {						//捡阳光球
	int w = imgSunBall[0].getwidth();					//easyx里面的
	int h = imgSunBall[0].getheight();					//用于得到图片的长与宽
	for (int i = 0; i < ballMax; i++) {          //需保证点击在阳光内才可以拾取      
		if (balls[i].used) {
			float x = balls[i].x;
			float y = balls[i].y;
			if (msg->message == WM_LBUTTONDOWN && msg->x > x && msg->x < x + w && msg->y > y && msg->y < y + h) {
				balls[i].click = true;
				//mciSendString("play res/audio/sunshine.mp3", 0, 0, 0);
				PlaySound("res/audio/sunshine.wav", NULL, SND_FILENAME | SND_ASYNC);
				balls[i].k = y / (x - 250 + 200);				//存储k值，即斜率
			}
		}
	}
}

void growPlants(ExMessage* msg) {		//鼠标种植植物
	if (msg->message == WM_LBUTTONDOWN && judgeShovel == false &&
		msg->x > 338 - 200 && msg->x < 338 - 200 + 65 * Cards && msg->y > 6 && msg->y < 96) {
		curPlants = (msg->x - 338 + 200) / 65 + 1;			//这里减去200
		if (cards[curPlants - 1].costable == true && cards[curPlants - 1].CDable == true) {
			judgePlants = true;					//正确点击后的标志，judgePlants在最开始定义过
			curX = msg->x;
			curY = msg->y;
			PlaySound("res/audio/select.wav", NULL, SND_FILENAME | SND_ASYNC);
		}
		else {
			PlaySound("res/audio/cannotselect.wav", NULL, SND_FILENAME | SND_ASYNC);
			curPlants = 0;
		}
	}
	else if (msg->message == WM_MOUSEMOVE && judgePlants == true && judgeShovel == false) {			//未种下植物跟随鼠标移动
		curX = msg->x;
		curY = msg->y;
	}
	else if (msg->message == WM_RBUTTONDOWN && judgePlants == true && judgeShovel == false) {		//取消种植
		PlaySound("res/audio/selectCancel.wav", NULL, SND_FILENAME | SND_ASYNC);
		judgePlants = false;
		curPlants = 0;
	}
	else if (msg->message == WM_LBUTTONDOWN && judgePlants == true && judgeShovel == false &&
		msg->x > curX00 && msg->x < WIN_Width && msg->y > curY00 && msg->y < WIN_Height) {
		int row = (msg->y - curY00) / cur_Height;		//行 y值
		int col = (msg->x - curX00) / cur_Width;		//列 x值
		int flag = 0;
		if (row >= 5 || col >= 9)				//看似只设置了9列，但是再往右放置植物的话会跑回第一列并在第一列放植物，故而在这里加个判断
			flag = 1;
		if (map[row][col].type == 0 && flag == 0) {
			sunSum -= cards[curPlants - 1].cost;
			map[row][col].type = curPlants;
			map[row][col].frameIndex = 0;
			map[row][col].x = curX00 + col * cur_Width;
			map[row][col].y = curY00 + row * cur_Height;		//储存该植物的xy坐标
			map[row][col].row = row;
			map[row][col].col = col;
			map[row][col].deadTime = 0;
			curPlants = 0;
			judgePlants = false;
			if (map[row][col].type == WallNut + 1) {			//根据所种植的植物更新特殊状态和CD
				cards[WallNut].CD = 550;
				map[row][col].status = WallNut_Normal;
			}
			else if (map[row][col].type == PotatoMine + 1) {
				cards[PotatoMine].CD = 550;						//大致18秒多
				map[row][col].status = PotatoMine_Under;
				map[row][col].prepareTime = 0;
			}
			else if (map[row][col].type == PeaShooter + 1) {
				cards[PeaShooter].CD = 250;						//大致8秒
			}
			else if (map[row][col].type == SunFlower + 1) {
				cards[SunFlower].CD = 250;
			}
			PlaySound("res/audio/plantdown.wav", NULL, SND_FILENAME | SND_ASYNC);
		}
	}
}

void clickShovel(ExMessage* msg) {			//铲子铲掉植物
	if (msg->message == WM_LBUTTONDOWN && msg->x > 660 && msg->x < 660 + 70 && msg->y > 0 && msg->y < 0 + 74 && judgePlants == false) {
		mciSendString("play res/audio/shovel.mp3", 0, 0, 0);
		curX = msg->x;
		curY = msg->y;
		judgeShovel = true;		//这个用来判断是否正在使用铲子
	}
	else if (msg->message == WM_MOUSEMOVE && judgeShovel == true && judgePlants == false) {		//点击铲子后铲子自动跟随鼠标
		curX = msg->x;
		curY = msg->y;
	}
	else if (msg->message == WM_RBUTTONDOWN && judgeShovel == true && judgePlants == false) {	//点击铲子后右键取消选择铲子
		PlaySound("res/audio/selectCancel.wav", NULL, SND_FILENAME | SND_ASYNC);
		judgeShovel = false;
	}
	else if (msg->message == WM_LBUTTONDOWN && judgeShovel == true && judgePlants == false &&
		msg->x > curX00 && msg->x < WIN_Width && msg->y > curY00 && msg->y < WIN_Height) {
		int row = (msg->y - curY00) / cur_Height;		//行 y值
		int col = (msg->x - curX00) / cur_Width;		//列 x值
		if (map[row][col].type > 0) {					//判断该草坪块有没有植物
			PlaySound("res/audio/plantdown.wav", NULL, SND_FILENAME | SND_ASYNC);
			judgeShovel = false;
			if (map[row][col].catched)
				map[row][col].shovelAndCatched = true;		//这个用于判断是否是正在被僵尸吃的时候铲掉植物
			else
				map[row][col].type = 0;		//如果不是那就正常铲（目前遗留一个小问题，就是在没有正在被僵尸吃的时候在铲掉，植物的状态好像还没更新）（似乎不用解决，或者根本没有这个问题）
		}									//这个状态更新应该是在下一个植物种植在这个草坪块的时候会进行更新
	}
}

void clickPause(ExMessage* msg) {
	if (msg->message == WM_LBUTTONUP && msg->x > 785 && msg->x < 785 + 113 && msg->y > 0 && msg->y < 0 + 42) {
		gameStatus = PAUSE;
		mciSendString("pause bg2", 0, 0, 0);
		mciSendString("play res/audio/pause.mp3", 0, 0, 0);
	}
}

//以下的是updateGame中的
void judgeCards() {						//判断阳光数是否足以购买植物以及卡牌是否冷却完毕
	for (int i = 0; i < Cards; i++) {
		if (cards[i].CD == 0) {
			cards[i].CDable = true;		//如果cd冷却完毕了那再考虑阳光够不够的问题
			if (sunSum >= cards[i].cost)
				cards[i].costable = true;
			else
				cards[i].costable = false;
		}
		else {
			cards[i].CDable = false;
			cards[i].CD--;				//如果cd没有完毕那就自减
		}
	}
}

void updatePlants() {		//更新植物状态(让植物随风摇摆起来)
	static int count = 0;							//减慢植物帧数播放速率
	count++;
	if (count > 1) {
		count = 0;
		for (int i = 0; i < 5; i++) {
			for (int j = 0; j < 9; j++) {
				if (map[i][j].type > 0) {
					map[i][j].frameIndex++;				//更新植物序列帧
					int plantsType = map[i][j].type - 1;
					int index = map[i][j].frameIndex;
					if (map[i][j].type == PotatoMine + 1 || map[i][j].type == WallNut + 1) {
						if (map[i][j].type == PotatoMine + 1 && map[i][j].status == PotatoMine_Under) {		//土豆地雷的判定
							map[i][j].prepareTime++;
							if (map[i][j].prepareTime > 300) {		//土豆地雷准备时间
								map[i][j].status = PotatoMine_Ready;
								map[i][j].prepareTime = 0;
							}
						}
						plantsType = map[i][j].status;
						if (imgPlantsStatus[plantsType][index] == NULL)
							map[i][j].frameIndex = 0;
					}
					else {		//这是输出豌豆射手和向日葵的图片返回判定
						if (imgPlants[plantsType][index] == NULL)
							map[i][j].frameIndex = 0;
					}
				}
			}
		}
	}
}

void createFlowerSun() {				//向日葵生产阳光（除了两个for循环其他基本上和createSun差不多）
	int sunFre;
	static int count[5][9];
	if (restart == true) {
		sunFre = 0;
		memset(count, 0, sizeof(count));
	}
	for (int i = 0; i < 5; i++) {
		for (int j = 0; j < 9; j++) {
			if (map[i][j].type == SunFlower + 1) {
				sunFre = 300 + rand() % 300;			//随机300到600
				count[i][j]++;
				if (count[i][j] > sunFre) {
					count[i][j] = 0;
					int k;
					for (k = 0; k < ballMax && balls[k].used; k++);

					if (k < ballMax) {		//向日葵生成的阳光球状态更新
						balls[k].flower = true;
						balls[k].used = true;
						balls[k].frameIndex = 0;
						balls[k].x = map[i][j].x;
						balls[k].y = map[i][j].y - 10;
						balls[k].destY = map[i][j].y + 20;
						balls[k].timer = 0;
						balls[k].click = false;
						balls[k].k = 0.0;
					}
				}
			}
		}			//新bug：第一列的向日葵产生阳光后捡起来后能触发音效且会消失，但是阳光值不会增加（已解决）
	}
}

void createSkySun() {
	static int count = 0;						//static变量：在下次进行调用该函数的时候，count的值不会再被更新为0
	int sunFre = 100 + rand() % 200;			//随机300到500
	count++;
	if (restart == true) {		//重开游戏时候清空数据
		sunFre = 0;
		count = 0;
	}
	if (count > sunFre) {
		count = 0;							//计数器count清零
		//从阳光池中取一个可以使用的
		int i;											//不写在for循环中是因为下面有俩i
		for (i = 0; i < ballMax && balls[i].used; i++); //判断是否是在池子中取的，并且取得的阳光球是否为已经被用过了（一个独立的for循环）

		if (i < ballMax && balls[i].flower == false)
		{
			//阳光球状态更新
			balls[i].used = true;
			balls[i].frameIndex = 0;
			balls[i].x = 300 + rand() % (800 - 300);		//取一个300到800之间的数
			balls[i].destY = 200 + (rand() % 4) * 90;		//掉落目标y值
			balls[i].y = 60;
			balls[i].timer = 0;
			balls[i].click = false;
			balls[i].k = 0.0;
		}									//如果阳光球s取完了就返回
	}
}

void updateSun() {
	for (int i = 0; i < ballMax; i++) {
		if (balls[i].used) {
			balls[i].frameIndex = (balls[i].frameIndex + 1) % 29;
			if (balls[i].timer == 0 && balls[i].click == false && balls[i].flower == false) {
				balls[i].y += 2;				//阳光球从天上不断下落，即y值不断增加，设定为2是控制速度
			}
			else if (balls[i].timer == 0 && balls[i].click == false && balls[i].flower == true) {
				balls[i].y += 1;				//向日葵生产的阳光球
			}
			if (balls[i].click == true) {		//阳光球被捡起后的动作
				balls[i].x -= 15;
				balls[i].y -= 15 * balls[i].k;
				balls[i].k = balls[i].y / (balls[i].x - 250 + 170);	//不断更新k值，因为不知道为啥如果不更新就到不了左上角
				if (balls[i].x < 250 - 170 && balls[i].y < 0) {
					sunSum += 25;
					balls[i].used = false;
					balls[i].flower = false;
				}
			}
			if (balls[i].y >= balls[i].destY || (balls[i].flower == true && balls[i].y >= balls[i].destY)) {	//如果已经到达要掉落的位置
				balls[i].timer++;				//开始计时
				if (balls[i].timer > 150) {		//阳光球在所掉落的位置保持150帧，方便玩家捡起
					balls[i].used = false;
					balls[i].flower = false;
				}
			}
		}
	}
}

void createZM() {
	if (zmCount >= zm_BeforeWave)
		return;
	static int zmFre = 500;
	static int count = 0;							//计数器
	static int flag = 1;
	count++;
	if (restart == true) {		//重开清空数据
		zmFre = 500;
		count = 0;
		flag = 1;
	}
	/*static int ABABA = 0;*/
	if (count > zmFre) {
		if (flag == 1) {
			mciSendString("play res/audio/zombiescoming.mp3", 0, 0, 0);
			flag = 0;
		}
		//ABABA++;
		//if (ABABA >= 3)		//只创造两个僵尸
		//	return;
		count = 0;
		int zmOut = rand() % 5;
		int zmOutFlag = 0;
		if (zmOut < 1 && zmOutFlag < 4) {	//生成多个僵尸的次数最多为3次
			zmFre = 50;						//一定概率生成多个僵尸
			zmOutFlag++;
		}
		else
			zmFre = rand() % 200 + 250;		//僵尸刷新时间间隔250到450，原来:rand() % 200 + 300
		int i;
		for (i = 0; i < zmMAX && zms[i].used; i++);//判断是否是在池子中取的，并且取得的僵尸是否为已经被用过了（一个独立的for循环）

		if (i < zmMAX) {//从僵尸池中取一个可以使用的
			//初始化僵尸的状态
			int random = rand() % 10;
			if (random <= 5) {
				zms[i].blood = 100;
				zms[i].type = NormalZM;
			}
			else if (random > 5 && random <= 8) {
				zms[i].blood = 200;
				zms[i].type = RoadConeZM;
			}
			else {
				zms[i].blood = 500;
				zms[i].type = BucketZM;
			}
			zms[i].row = rand() % 4;				//行（这里的4，指的是取0到4的随机数）原本为：rand() % 4 为了测试改成2
			zms[i].y = 72 + (1 + zms[i].row) * 100;	//BUG解决：原本这里是172，而这样的话第一行就不会有僵尸了，改成72就可
			zms[i].used = true;
			zms[i].x = WIN_Width - 60;
			zms[i].speed = 1;
			zms[i].dead = false;
			zms[i].eating = false;
			zms[i].frameIndex = 0;
			zmCount++;
		}
	}
}

void updateZM() {
	//更新僵尸的位置
	static int count = 0;
	count++;
	if (count > 2) {			//减慢僵尸速度（减半）
		count = 0;
		for (int i = 0; i < zmMAX; i++) {
			if (zms[i].used) {
				zms[i].x -= zms[i].speed;
				if (zms[i].x < curX00 - 120) {
					gameStatus = FAIL;
				}
			}
		}
	}
	static int count2 = 0;
	count2++;
	if (count2 > 6) {			//减慢僵尸帧数播放速率
		count2 = 0;
		for (int i = 0; i < zmMAX; i++) {
			if (zms[i].used) {
				if (zms[i].dead) {		//判断僵尸死亡
					zms[i].frameIndex++;
					if (zms[i].frameIndex >= 10) {	//如果僵尸死亡就增加死亡的帧数，并在加完之后删除僵尸
						zms[i].used = false;
						killCount++;
						if (killCount == zm_BeforeWave)
							gameStatus = WIN;
					}
				}
				else if (zms[i].eating) {	//如果正在吃那么就增加吃的帧数
					if (zms[i].type == NormalZM)
						zms[i].frameIndex = (zms[i].frameIndex + 1) % 21;
					else if (zms[i].type == RoadConeZM)
						zms[i].frameIndex = (zms[i].frameIndex + 1) % 11;
					else if (zms[i].type == BucketZM)
						zms[i].frameIndex = (zms[i].frameIndex + 1) % 11;
				}
				else {					//如果没死就正常地增加帧数
					if (zms[i].type == NormalZM)
						zms[i].frameIndex = (zms[i].frameIndex + 1) % 22;
					else if (zms[i].type == RoadConeZM)
						zms[i].frameIndex = (zms[i].frameIndex + 1) % 21;
					else if (zms[i].type == BucketZM)
						zms[i].frameIndex = (zms[i].frameIndex + 1) % 15;
				}
			}
		}
	}
}

void createShoot() {					//豌豆发射子弹
	int lines[5] = { 0 };				//每一行判断是否有僵尸（这个自我感觉可以优化，但是不知道怎么整）
	for (int i = 0; i < zmMAX; i++) {
		if (zms[i].used) {
			lines[zms[i].row] = 1;
		}
	}
	for (int i = 0; i < 5; i++) {		//遍历草坪块
		for (int j = 0; j < 9; j++) {
			if (map[i][j].type == PeaShooter + 1 && lines[i] == 1) {
				static int count[5][9] = { 0 };	//降低豌豆子弹发射频率
				count[i][j]++;
				if (count[i][j] > 60) {
					count[i][j] = 0;
					int k;
					for (k = 0; k < bulletMax && bullets[k].used; k++);//独立for循环，不多讲

					if (k < bulletMax) {
						bullets[k].row = i;
						bullets[k].speed = 8;
						bullets[k].used = true;
						bullets[k].blast = false;
						bullets[k].framesIndex = 0;
						int zwX = curX00 + j * cur_Width;	//植物的坐标
						int zwY = curY00 + i * cur_Height;
						bullets[k].x = zwX + imgPlants[map[i][j].type - 1][0]->getwidth() - 10;//植物帧图片的宽度
						bullets[k].y = zwY + 5;				//这里的x和y是开火位置
						int flag = rand() % 2;				//随机播放这两个音效中的一个
						if (flag == 2)
							PlaySound("res/audio/shootpea.wav", NULL, SND_FILENAME | SND_ASYNC);
						else
							PlaySound("res/audio/shootpea2.wav", NULL, SND_FILENAME | SND_ASYNC);
					}
				}
			}
		}
	}
}

void updateshoot() {		//更新豌豆子弹，课程视频里叫做updateBullets
	for (int i = 0; i < bulletMax; i++) {
		if (bullets[i].used) {
			bullets[i].x += bullets[i].speed;
			if (bullets[i].x > WIN_Width) {		//这里是因为在僵尸死亡后之时，豌豆射手还是会多发射一个子弹，这个可以回收没有打在僵尸身上的子弹
				bullets[i].used = false;
			}
			if (bullets[i].blast) {		//子弹的碰撞检测
				bullets[i].framesIndex++;
				if (bullets[i].framesIndex >= 4) {
					bullets[i].used = false;
				}
			}
		}
	}
}

void checkBullets2ZM() {					//子弹和僵尸的碰撞检测以及僵尸死亡检测
	for (int i = 0; i < bulletMax; i++) {
		if (bullets[i].used == false || bullets[i].blast == true)
			continue;		//这里不直接把if判断放在for循环里面是因为有可能第1个子弹已经击中，而第2个子弹已经发射但未击中,而如果放在for循环中，那就在判断第1个子弹已经为击中的情况下之后不会再判断接下来的子弹了
		for (int k = 0; k < zmMAX; k++) {
			if (zms[k].used == false || bullets[i].row != zms[k].row)		//这里同理（用到continue也可以说明问题）
				continue;
			int Dx = zms[k].x + 75;			//Dx是僵尸的碰撞点的x坐标值
			int x = bullets[i].x;
			if (zms[k].dead == false && x > Dx) {
				if (zms[k].type == NormalZM) {
					int flag = rand() % 2;				//随机播放这三个音效中的一个
					if (flag == 2)
						PlaySound("res/audio/peacrush.wav", NULL, SND_FILENAME | SND_ASYNC);
					else if (flag == 1)
						PlaySound("res/audio/peacrush2.wav", NULL, SND_FILENAME | SND_ASYNC);
					else
						PlaySound("res/audio/peacrush3.wav", NULL, SND_FILENAME | SND_ASYNC);
				}
				else if (zms[k].type == RoadConeZM) {
					int flag = rand() % 2;
					if (flag == 2)
						PlaySound("res/audio/peaToRoadCone1.wav", NULL, SND_FILENAME | SND_ASYNC);
					else
						PlaySound("res/audio/peaToRoadCone2.wav", NULL, SND_FILENAME | SND_ASYNC);
				}
				else if (zms[k].type == BucketZM) {
					int flag = rand() % 2;
					if (flag == 2)
						PlaySound("res/audio/peaToBucket1.wav", NULL, SND_FILENAME | SND_ASYNC);
					else
						PlaySound("res/audio/peaToBucket2.wav", NULL, SND_FILENAME | SND_ASYNC);
				}
				zms[k].blood -= 10;			//僵尸扣血(原本为10）
				bullets[i].blast = true;
				bullets[i].speed = 0;
				if (zms[k].blood <= 0) {	//死亡判断
					zms[k].dead = true;
					zms[k].speed = 0;
					zms[k].frameIndex = 0;
				}
				break;		//这个break是在里面的这个for循环中
			}
		}
	}
}

void checkPotatoMine2ZM() {			//土豆地雷对僵尸的判定
	for (int i = 0; i < 5; i++) {
		for (int j = 0; j < 9; j++) {
			if (map[i][j].type == PotatoMine + 1 && map[i][j].status == PotatoMine_Ready) {
				for (int k = 0; k < zmMAX; k++) {
					if (zms[k].used == false || zms[k].dead || zms[k].row != map[i][j].row)
						continue;
					int plantX = 256 - 120 + j * 81;
					int x1 = plantX + 10;				//x1和x2是土豆地雷的边界
					int x2 = plantX + 60;
					int x3 = zms[k].x + 80;				//x3是僵尸的左边界
					if (zms[k].type == RoadConeZM)	//专门应对路障僵尸图片像素左边太多导致路障僵尸还没到地方就开始吃
						x3 += 20;
					if (x3 > x1 && x3 < x2) {
						zms[k].used = false;
						map[i][j].boom = true;
						killCount++;
						mciSendString("play res/audio/potato_mine.mp3", 0, 0, 0);
					}
				}
			}
		}
	}
}

void plantsDie(int row, int k) {			//判断植物的死亡是被铲掉还是被吃掉，并更新草坪块和僵尸的状态
	if (map[row][k].shovelAndCatched == true)
		PlaySound("res/audio/plantdown.wav", NULL, SND_FILENAME | SND_ASYNC);
	else
		mciSendString("play res/audio/plantDead.mp3", 0, 0, 0);
	for (int p = 0; p < zmMAX; p++) {		//这个for循环遍历所有僵尸，可以解决两个僵尸同时吃完同一个植物后会有一个僵尸一直保持吃的状态的bug
		if (zms[p].eating == true) {
			zms[p].eating = false;
			zms[p].frameIndex = 0;
			zms[p].speed = 1;
		}
	}
	map[row][k].type = 0;		//重新更新草坪块和僵尸的状态
	map[row][k].deadTime = 0;
	map[row][k].catched = false;
	map[row][k].shovelAndCatched = false;
}

void checkZM2Plants() {				//僵尸吃植物的碰撞检测
	for (int i = 0; i < zmMAX; i++) {
		if (zms[i].used) {
			for (int k = 0; k < 9; k++) {			//这一行的九个草坪块
				int row = zms[i].row;
				if (map[row][k].type == 0)
					continue;					//如果这个草坪块没有植物就略过
				int plantX = 256 - 120 + k * 81;
				int x1 = plantX + 10;				//x1和x2是植物的边界
				int x2 = plantX + 60;
				int x3 = zms[i].x + 80;				//x3是僵尸的左边界
				static int count[zm_BeforeWave] = { 0 };
				if (zms[i].type == RoadConeZM)	//专门应对路障僵尸图片像素左边太多导致路障僵尸还没到地方就开始吃
					x3 += 20;
				if (x3 > x1 && x3 < x2) {
					if (zms[i].dead) {				//这个用于解决僵尸正在吃植物的时候死掉后该植物无法再用铲子铲掉的问题
						map[row][k].catched = false;
						map[row][k].shovelAndCatched = false;
						continue;
					}
					if (map[row][k].catched && zms[i].eating) {		//判断是否已经被在被吃，意思就是要先走一遍下面的else然后在走这个if（加了个zms[i].eating可以让多个僵尸同时吃一个植物）
						if (map[row][k].type == PotatoMine + 1 && map[row][k].status == PotatoMine_Ready)
							return;					//这个尝试解决僵尸遇到土豆地雷有时候会出现还是会吃掉的问题
						map[row][k].deadTime++;
						count[i]++;
						if (count[i] > 20) {		//这个还是有点用的，就是如果代码走到这里的频率太高了，那么就无论如何都不会播放这个音效了，所以加个这个
							count[i] = 0;
							mciSendString("play res/audio/zmeat.mp3", 0, 0, 0);//播放音效
						}
						if (map[row][k].type == WallNut + 1) {//判断植物是不是坚果墙（和下面的几乎一样，但是不知道怎么节省空间）
							if (map[row][k].deadTime <= 400)
								map[row][k].status = WallNut_Normal;
							else if (map[row][k].deadTime > 400 && map[row][k].deadTime <= 700)
								map[row][k].status = WallNut_Cracked;
							else if (map[row][k].deadTime > 700 && map[row][k].deadTime <= 1000)
								map[row][k].status = WallNut_Cracked2;
							if (map[row][k].deadTime > 1000 || map[row][k].shovelAndCatched == true) {
								plantsDie(row, k);		//判断植物的死亡是被铲掉还是被吃掉，并更新草坪块和僵尸的状态
							}
						}
						else {
							if (map[row][k].deadTime > 150 || map[row][k].shovelAndCatched == true) {		//这里是判断植物的死亡方式，即是被铲掉的还是被吃掉的
								plantsDie(row, k);		//判断植物的死亡是被铲掉还是被吃掉，并更新草坪块和僵尸的状态
							}
						}
					}
					else {	//判断是否第一次被吃，第一次被吃就会走下面这个else，然后在第二次被吃的时候走上面那个if，之后一直都会走上面的if直到吃完
						map[row][k].catched = true;		//这样就会出现一个bug，那就是无法两个僵尸同时吃一个植物（已解决，在上面的if语句中加了个判断僵尸是不是在吃）
						zms[i].eating = true;
						zms[i].speed = 0;
						zms[i].frameIndex = 0;
					}
				}
			}
		}
	}
}

void drawCards() {			//渲染卡槽中的植物
	for (int i = 0; i < Cards; i++) {
		int x = 338 - 200 + i * 65;				//这里减去200
		int y = 6;
		if (cards[i].costable == true && cards[i].CDable == true)		//设定是，如果阳光数足够且cd也冷却完毕，那就输出正常图片
			putimagePNG(x, y, &imgCards[i]);
		else if (cards[i].CDable == false)								//如果cd冷却未完毕，那么无论够不够买植物，都输出冷却中图片
			putimagePNG(x, y, &imgCardsCD[i]);
		else if (cards[i].costable == false && cards[i].CDable == true)//如果cd冷却完毕但是不够买植物，那就输出黑白图片
			putimagePNG(x, y, &imgCardsBlack[i]);
	}
}

void drawPlants() {				//渲染植物
	for (int i = 0; i < 5; i++) {
		for (int j = 0; j < 9; j++) {
			if (map[i][j].type > 0) {
				int plantsType = map[i][j].type - 1;
				int index = map[i][j].frameIndex;
				if (map[i][j].type == WallNut + 1) {			//用imgPlantsStatus专门用来输出坚果墙的三种状态
					if (map[i][j].status == WallNut_Normal)
						plantsType = WallNut_Normal;
					else if (map[i][j].status == WallNut_Cracked)
						plantsType = WallNut_Cracked;
					else if (map[i][j].status == WallNut_Cracked2)
						plantsType = WallNut_Cracked2;
					if (imgPlantsStatus[plantsType][index] == NULL) {//补丁，用于修复坚果墙吃到破损时候有几率崩溃，虽然这个判断应该在updatePlants，
						map[i][j].frameIndex = 0;					 //但是由于代码顺序它是先进行输出图片，之后再进行判断（我打赌这个bug真的真的被修复了！）
						index = map[i][j].frameIndex;
					}
					putimagePNG(map[i][j].x, map[i][j].y, imgPlantsStatus[plantsType][index]);
				}
				else if (map[i][j].type == PotatoMine + 1) {
					if (map[i][j].status == PotatoMine_Under)
						plantsType = PotatoMine_Under;
					else if (map[i][j].status == PotatoMine_Ready)
						plantsType = PotatoMine_Ready;
					if (imgPlantsStatus[plantsType][index] == NULL) {		//补丁同上
						map[i][j].frameIndex = 0;
						index = map[i][j].frameIndex;
					}
					putimagePNG(map[i][j].x, map[i][j].y, imgPlantsStatus[plantsType][index]);
				}
				else {
					putimagePNG(map[i][j].x, map[i][j].y, imgPlants[plantsType][index]);
				}
				if (map[i][j].type == PotatoMine + 1 && map[i][j].boom) {
					map[i][j].prepareTime++;
					if (map[i][j].prepareTime < 70) {		//土豆地雷的爆炸后输出的图片，持续一定时间
						putimagePNG(map[i][j].x - 25, map[i][j].y - 25, &imgboom[0]);
						putimagePNG(map[i][j].x - 25, map[i][j].y - 15, &imgboom[1]);
					}
					else {						//输出结束后更新草坪块状态
						map[i][j].boom = false;
						map[i][j].prepareTime = 0;
						map[i][j].type = 0;			//这里感觉应该放在checkPotatoMine2ZM里面的
					}
				}
			}
		}
	}
	if (judgePlants == true)					//渲染拖动过程中的植物
		putimagePNG(curX - 32, curY - 32, imgPlants[curPlants - 1][0]); //减去32是为了再选中时改变植物位置，比老师教的简化些
}

void drawShovel() {				//渲染未拖动的以及拖动过程中的铲子
	if (judgeShovel == true)
		putimagePNG(curX - 32, curY - 32, &imgShovel);
	else
		putimagePNG(660, 0, &imgShovel);		//铲子70像素宽
}

void drawSunBalls() {				//根据帧数渲染阳光球图片
	for (int i = 0; i < ballMax; i++) {
		if (balls[i].used == true) {
			putimagePNG(balls[i].x, balls[i].y, &imgSunBall[balls[i].frameIndex]);
		}
	}
}

void drawZM() {					//根据帧数渲染僵尸图片
	for (int i = 0; i < zmMAX; i++) {
		if (zms[i].used) {
			int zmsType = zms[i].type;
			int index = zms[i].frameIndex;
			if (zms[i].dead)
				putimagePNG(zms[i].x, zms[i].y - 144/*- &imgZMDead[index]->getheight()*/, &imgZMDead[index]);
			else if (zms[i].eating)
				putimagePNG(zms[i].x, zms[i].y - 144, imgZMEat[zmsType][index]);
			else 
				putimagePNG(zms[i].x, zms[i].y - 144, imgZM[zmsType][index]);
		}
	}
}

void drawBullets() {							//渲染豌豆子弹
	for (int i = 0; i < bulletMax; i++) {
		if (bullets[i].used) {
			if (bullets[i].blast)
				putimagePNG(bullets[i].x, bullets[i].y, &imgBulletBlast[bullets[i].framesIndex]);
			else
				putimagePNG(bullets[i].x, bullets[i].y, &imgBulletNormal);
		}
	}
}

void drawSunNumber() {//输出字体（阳光数字）
	char scoreTest[8];
	sprintf_s(scoreTest, sizeof(scoreTest), "%d", sunSum);
	outtextxy(277 - 200, 67, scoreTest);
}