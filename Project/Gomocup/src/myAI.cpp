#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <string.h>
#include <conio.h>
#include <windows.h>
#include <math.h>
#include <float.h>
#include <iostream>
#include "myAI.h"
using namespace std;



int tell=0;
//---------------------------------------debug--------------------------------------------//
bool engine=1;  //是否为计算引擎状态
/* int printmain=0;
int printmain2=0;
int printmain3=0; */
/* int debuging=0;
int debuging2=0;
int debuging3=0;
int debuging_wherebug=0;
int debuging_draw_new_thinking=0; */

//--------------------------------------重要参数-------------------------------------------//

int sizex=20;   //棋盘宽
int sizey=20;   //棋盘长

int step=0;         /*实际步数*/

#define n_VCF_MAX  3        //以下一组数据中的最大值（用于定义数组维数）
int n_VCF_d1_1=3;       //VCF第一步（当前局面）尝试点数
int n_VCF_d1_2=1;       //VCF第一步（试探的board）尝试点数
int n_VCF_d2_1=1;       //VCF深度为1时尝试点数
int n_VCF_d2_2=1;       //VCF深度为2时尝试点数
int n_VCF_d2_3=1;       //VCF深度>=3时尝试点数


int timeout_turn=8000; /* time for one turn in milliseconds */
int timeout_match=1000000000; /* total time for a game */
int timeout_left=1000000000; /* left time for a game */
int max_memory=0; /* maximum memory in bytes, zero if unlimited */

int engine_start_time=0;    /*计算引擎状态，读取的实际开始计时时间*/
int engine_timeout_left=0;  /*计算引擎状态，读取的实际剩余时间*/

int timeout_gap=80;        /*该步思考剩余时间小于该值（平均限时）时，停止继续尝试，尽快给出走法*/
int timeout_gap_real=160;   /*该步思考剩余时间小于该值（严格限时）时，停止继续尝试，尽快给出走法*/
int timeout_num=12;     /*timeout_left/timeout_num作为timeout_turn的上限*/ //暂时优化：11,12
int quickanswer=0;          /*时间限制即将达到时，取为1*/

int n_AI=0;     //restart后第几次调用AI
int firstAI_time_cost=300;  //engine状态下第一次调用AI，为避免因初始化而超时，降低时间上限

DWORD oldtime=0;
DWORD timeelapsed=0;

int total_node=0;   //总node数统计
int maxn_node_each=1000000000;  //单次计算允许的最大node数
int value_depth=18;     //暂时优化：15
float ratio_bw=4.0;
#define MAX_KEEP_UPBOUND 10     //用于定义数组
int max_keep=6; //完成depth为1的计算后，最多保留当前最优的若干点 //暂时优化：6,4
int max_gap=300;    //完成depth为1的计算后，最多保留比最优价值低max_gap的点
int max_gap_return=1000;    //若最优走法与次优走法value差距大于该值，立即结束计算

bool sym_random=0;  //对称的走法以相等的概率随机选取。（略微浪费时间，对计算速度要求很高时，建议设为0）

//----------------------------------随机函数--------------------------------//
float random()
{
    int r1,r2,r3,r4;
    float r;
    r1=rand();
    r2=rand();
    r3=rand();
    r4=rand();
    r=(float)r1/32768.0+(float)r2/32768.0/32768.0+(float)r3/32768.0/32768.0/32768.0+(float)r4/32768.0/32768.0/32768.0/32768.0;
    return r;
}

//---------------------------------基本无需更改的定义---------------------------------------//
#define MAXSIZE 20          /*棋盘尺寸*/            //建议根据实际需要进行修改
#define NUMMOST MAXSIZE*MAXSIZE /*最多棋子数*/

struct Point                /*点*/
{
    int x;
    int y;
};

struct Board                /*局面*/
{
    int whosechess;             /*真实棋盘：0未下1黑2白4边界；临时棋盘：0未下1己方2对方3待下4边界（或双方禁下点）*/
    int step;                   /*第几步下在此处*/
    float value;                /*目前该处棋子价值*/
    bool block;                 /*1屏蔽该点0不屏蔽*/
    bool abandon;               /*1:搜索棋形qxa1等时，放弃这些点*/
}realboard[MAXSIZE+2][MAXSIZE+2];


struct Qixing               /*棋形*/
{
    char qx[16];
    float value;
};
int direction[8][2]={{0,-1},{0,1},{-1,0},{1,0},{-1,-1},{1,1},{-1,1},{1,-1}};    /*方向*/
int ndirection[4][2]={{1,0},{0,1},{1,1},{1,-1}};    /*方向*/


//-----------------------------------棋形--------------------------------------//

Qixing qxa1[]={{"11113",1},{"11131",1},{"11311",1},{"\0",0}};//注意具有对称性的需要减半
Qixing qxb2[]={{"022223",1},{"222023222",1},{"22022322",1},{"2022232",1},{"\0",0}};
Qixing qxc2[]={{"322221",1},{"322224",1},{"22232",1},{"22322",1},{"\0",0}};
Qixing qxb1[]={{"011130",1},{"011310",1},{"111030111",1},{"11031011",1},{"1031101",1},{"1013101",1},{"\0",0}};
Qixing qxc1[]={{"11103",1000},{"11130",1000},{"11013",1000},{"11310",1000},{"11301",1000},{"11031",1000},
            {"10113",1000},{"13110",1000},{"10131",1000},{"01113",1000},{"\0",0}};
Qixing qxd2[]={{"0022230",1001},{"1022230",1101},{"4022230",1101},{"1322200",1001},{"4322200",1001},{"1022203",1001},{"4022203",1001},
            {"022320",1101},{"322020",1011},{"00022023",1001},{"1022023",1006},{"4022023",1006},{"10022023",1006},{"40022023",1006},
            {"222030222",1000},{"222300222",1000},{"22002322",1000},{"22032022",1000},{"22302022",1000},{"2032202",1000},{"2302202",1000},
            {"2002232",1000},{"2023202",1000},{"2020232",1000},{"\0",0}};
Qixing qxd1[]={{"011300",1000},{"001130",1000},{"011030",1000},{"013100",1000},
            {"010130",1000},{"013010",1000},{"1010301",1000},{"21013001",1000},{"41013001",1000},{"21031001",1000},
            {"41031001",1000},{"211030011",1000},{"411030011",1000},{"\0",0}};

Qixing qxe1[]={{"211300",400},{"411300",400},{"211030",400},{"411030",400},{"211003",400},{"411003",400},
            {"213100",400},{"413100",400},{"2101302",400},{"4101302",400},{"2101304",400},{"4101304",400},
            {"2101032",400},{"4101032",400},{"2101034",400},{"4101034",400},{"213010",400},{"413010",400},
            {"2103102",400},{"4103102",400},{"2103104",400},{"4103104",400},{"100132",400},{"100134",400},
            {"231100",400},{"431100",400},{"2011302",400},{"4011302",400},{"2011304",400},{"4011304",400},
            {"2011032",400},{"4011032",400},{"2011034",400},{"4011034",400},{"231010",400},{"431010",400},
            {"2013102",200},{"4013102",400},{"2013104",400},{"4013104",200},{"21013002",400},{"41013002",400},
            {"21013004",400},{"41013004",400},{"20101032",400},{"40101032",400},{"20101034",400},{"40101034",400},
            {"21010302",400},{"41010302",400},{"21010304",400},{"41010304",400},{"201010302",400},{"401010302",400},
            {"201010304",400},{"401010304",400},{"21031002",400},{"41031002",400},{"21031004",400},{"41031004",400},
            {"1001302",400},{"1001304",400},{"20103012",400},{"40103012",400},{"20103014",400},{"40103014",400},
            {"201030102",200},{"401030102",400},{"201030104",400},{"401030104",200},{"1003102",400},{"1003104",400},
            {"21013000",450},{"41013000",450},{"21010300",450},{"41010300",450},{"201010300",450},{"401010300",450},
            {"001010302",450},{"001010304",450},{"21031000",450},{"41031000",450},{"1001300",450},{"0013001",450},
            {"21030100",450},{"41030100",450},{"201030100",450},{"401030100",450},{"1003100",450},{"001010300",500},
            {"001030100",250},{"\0",0}};
Qixing qxf1[]={{"00013000",250},{"0010300",250},{"010030",200},{"10003",30},{"00013002",240},{"00013004",240},
            {"0010302",235},{"0010304",235},{"010032",30},{"010034",30},{"20013000",240},{"40013000",240},
            {"23010000",30},{"43010000",30},{"2031000",220},{"4031000",220},{"231000",20},{"431000",20},
            {"2013000",220},{"4013000",220},{"2010300",235},{"4010300",235},{"213000",20},{"413000",20},
            {"210300",30},{"410300",30},{"210030",30},{"410030",30},{"20013002",235},{"40013002",235},
            {"20013004",235},{"40013004",235},{"23010002",30},{"43010002",30},{"23010004",30},{"43010004",30},
            {"2001302",30},{"4001302",30},{"2001304",30},{"4001304",30},{"2001032",30},{"4001032",30},
            {"2001034",30},{"4001034",30},{"2013002",30},{"4013002",30},{"2013004",30},{"4013004",30},
            {"2010302",30},{"4010302",30},{"2010304",30},{"4010304",30},{"\0",0}};
Qixing qxe2[]={{"122230",650},{"422230",650},{"122203",650},{"422203",650},{"122320",650},{"422320",650},
            {"122023",650},{"422023",650},{"22302",650},{"122032",650},{"422032",650},{"1232201",650},
            {"4232201",650},{"1232204",650},{"4232204",650},{"120223",650},{"420223",650},{"12322001",650},
            {"42322001",650},{"12322004",650},{"42322004",650},{"12322000",640},{"42322000",640},{"232021",650},
            {"232024",650},{"2320201",650},{"2320204",650},{"2320200",650},{"1230221",650},{"4230221",650},
            {"1230224",650},{"4230224",650},{"12302201",650},{"42302201",650},{"12302204",650},{"42302204",650},
            {"123022001",640},{"423022001",640},{"123022004",640},{"423022004",640},{"123022000",640},{"423022000",640},
            {"1022032",650},{"4022032",650},{"10022032",640},{"40022032",640},{"00022032",640},{"2023202",750},
            {"2320202",1300},{"2022302",1500},{"2322002",1500},{"12022032",1500},{"42022032",1500},{"\0",0}};
Qixing qxf2[]={{"00322000",500},{"0322002",500},{"10322000",500},{"40322000",500},{"1322000",500},{"4322000",500},{"0322001",530},
            {"1322001",100},{"4322001",100},{"0322004",530},{"1322004",100},{"4322004",100},{"1302201",100},
            {"4302201",100},{"0302201",510},{"1302204",100},{"4302204",100},{"0302204",260},{"03022000",420},
            {"03022001",480},{"03022004",480},{"300221",100},{"300224",100},{"1032201",260},{"4032201",260},
            {"0032201",510},{"1032204",260},{"4032204",260},{"0032204",510},
            {"030221",100},{"030224",100},{"003221",100},{"003224",100},{"0023200",250},{"1023200",490},
            {"4023200",490},{"123200",120},{"423200",120},{"1023201",50},{"4023201",100},{"1023204",100},
            {"4023204",50},{"320200",500},{"320201",280},{"320204",280},{"302021",130},{"302024",130},
            {"032021",130},{"032024",130},{"023020",500},{"023021",130},{"023024",130},{"123020",130},
            {"423020",130},{"320020",250},{"320021",130},{"320024",130},{"20302",65},{"23002",130},{"\0",0}};
Qixing qxg1[]={{"40000003",1},{"4000003",2},{"400003",3},{"40003",4},{"4003",7},{"403",15},{"43",20},{"\0",0}};
Qixing qxg2[]={{"00320000",8},{"00320001",8},{"00320004",20},{"0032001",10},{"0032004",10},{"003201",5},
            {"003204",5},{"000321",1},{"000324",1},{"10320000",1},{"40320000",1},{"10320001",1},
            {"40320001",1},{"10320004",1},{"40320004",1},{"1032001",1},{"4032001",1},{"1032004",1},
            {"4032004",1},{"1320000",1},{"4320000",1},{"1320001",1},{"4320001",1},{"1320004",1},
            {"4320004",1},{"00302000",5},{"00302001",10},{"00302004",10},{"0030201",5},{"0030204",5},
            {"003021",1},{"003024",1},{"03002000",1},{"03002001",2},{"03002004",2},{"0300201",5},
            {"0300204",5},{"030021",1},{"030024",1},{"10302000",3},{"40302000",3},{"10302001",8},
            {"40302001",8},{"10302004",8},{"40302004",8},{"1030201",1},{"4030201",1},{"1030204",1},
            {"4030204",1},{"1302000",1},{"4302000",1},{"1302001",1},{"4302001",1},{"1302004",1},
            {"4302004",1},{"13002001",1},{"43002001",1},{"13002004",1},{"43002004",1},{"1300201",1},
            {"4300201",1},{"1300204",1},{"4300204",1},{"\0",0}};

//以上是对点判断可形成的棋形。以下是对棋盘判断已有棋形。
/*
【价值表（棋盘已有局面，不计入己方即将落的子）】

己方已成五子  必胜（value_win=10000）
对方已成五子  必败（-value_win=-10000）
己方活四    必胜（value_win=10000）
对方活四    必败（-value_win=-10000）
对方冲四    走法确定，不作计算（或落子后计算新局面）

=============== 以下不再考虑上述简单必胜必败情形 ==============


（双方无冲四，己方必无活三）  己方有眠三（较被动，但可能有VCF或用冲四堵活三的好棋）                                己方无眠三（被动）
对方已有的活三                     每个value_c2=30 /attackratio^(主动程度)                           仅一个时value_c2_d1F=60【取负值】；两个以上必败【不再考虑】


    根据各棋形的有无判断局势优劣：

    1.(对方无冲四，己方必无活三)若对方有活三：
    （1）若己方有眠三：  若对方仅一个活三，主动程度-1；若对方至少两个活三，主动程度-4
    （2）若己方无眠三：  若对方有眠三或活二-->    主动程度-2
                若对方无眠三或活二-->    若己方有活二，主动程度0；否则，主动程度-2
        另外，若对方活三数>=2，主动程度减5.
    2.(对方无冲四无活三，己方必无活三）若己方有眠三：
    （1）若对方有眠三：  主动程度1
    （2）若对方无眠三：  若己方有活二，主动程度2；否则，主动程度1
    3.(对方无冲四无活三，己方无活三无眠三）若对方有眠三：
    （1）若己方有活二：  主动程度0
    （2）若己方无活二：  主动程度-2
    另外，若对方仅有一个眠三且无活二，则主动程度加1.
    4.(对方无冲四无活三无眠三，己方无活三无眠三）若己方有活二：
    （1）若对方有活二：  主动程度1
    （2）若对方无活二：  主动程度3
    5.(对方无冲四无活三无眠三，己方无活三无眠三无活二）若对方有活二：
    （1）若己方有眠二：  主动程度-1
    （2）若己方无眠二：  主动程度-2
    另外，若对方仅有一个活二，则主动程度加1.
    6.(对方无冲四无活三无眠三无活二，己方无活三无眠三无活二）若己方有眠二：
    （1）若对方有眠二：  主动程度1
    （2）若对方无眠二：  主动程度2
    7.(对方无冲四无活三无眠三无活二，己方无活三无眠三无活二无眠二）若对方有眠二：
    （1）若己方有活一：  主动程度0
    （2）若己方无活一：  主动程度-2
    8.(对方无冲四无活三无眠三无活二无眠二，己方无活三无眠三无活二无眠二）若己方有活一：
    （1）若对方有活一：  若己方活一数>=对方活一数-->    主动程度2
                若己方活一数<对方活一数--> 主动程度0
    （2）若对方无活一：  主动程度3
    9.(对方无冲四无活三无眠三无活二无眠二，己方无活三无眠三无活二无眠二无活一）若对方有活一：
    （1）若对方仅一个活子：        主动程度-1
    （2）若对方至少两个活子：   主动程度-2
己方已有的眠三 每个value_d1=30 *attackratio^(主动程度)
对方已有的眠三 每个value_d2=30 /attackratio^(主动程度)【取负值】
己方已有的活二 每个value_e1=20 *attackratio^(主动程度)
对方已有的活二 每个value_e2=20 /attackratio^(主动程度)【取负值】
己方已有的眠二 每个value_f1=5 *attackratio^(主动程度)
对方已有的眠二 每个value_f2=5 /attackratio^(主动程度)【取负值】
己方已有的活一 每个 (1至3)*attackratio^(主动程度)
对方已有的活一 每个 (1至3)/attackratio^(主动程度)【取负值】
attackratio=sqrt(2)
*/
//（计数时同“行”同方向的己方、对方棋形分别最多计一个，即价值最高的一个。）
#define value_UNCOMPUTED 1111
#define value_ABANDON -20000
#define value_win       10000   //必胜方对应价值
#define value_b2        15      //对方（有且仅有）一个冲四的价值
#define value_c1        30      //己方活三的默认价值
#define value_c2        30      //对方活三的默认价值
#define value_d1        30      //己方眠三的默认价值
#define value_d2        30      //对方眠三的默认价值
#define value_e1        20      //己方活二的默认价值
#define value_e2        20      //对方活二的默认价值
#define value_f1        5       //己方眠二的默认价值
#define value_f2        5       //对方眠二的默认价值
//float attackratio=sqrt(2.0);
    float attackratio=sqrt(2.0);

/*
Qixing nqxw1[]={{"11111",value_win},{"\0",0}};//己方成五子
Qixing nqxw2[]={{"22222",value_win},{"\0",0}};//对方成五子
Qixing nqxab1[]={{"11110",value_win},{"11101",value_win},{"11011",value_win},{"10111",value_win},{"01111",value_win},{"\0",0}}; //己方活四、冲四
Qixing nqxa2[]={{"022220",value_win},{"222020222",value_win},{"22022022",value_win},{"2022202",value_win},{"\0",0}};        //对方活四或相当于活四
//以上为必胜或必败
Qixing nqxb2[]={{"0022202",value_b2+value_d2},{"2022200",value_b2+value_d2},{"00022022000",value_b2+value_f2*2},{"00022022",value_b2+value_f2},{"22022000",value_b2+value_f2},\
            {"22220",value_b2},{"22202",value_b2},{"22022",value_b2},{"20222",value_b2},{"02222",value_b2},{"\0",0}};   //对方冲四
Qixing nqxc1[]={{"0011100",value_c1+value_d1},{"0111000111",value_c1+value_d1},{"1110001110",value_c1+value_d1},{"110010110",value_c1+value_d1},{"011010011",value_c1+value_d1},\
            {"10011010",value_c1+value_d1},{"01011001",value_c1+value_d1},{"00011001011",value_c1+value_e1},{"11010011000",value_c1+value_e1},\
            {"00101010100",value_c1+value_f1*2},{"001010101",value_c1+value_f1},{"101010100",value_c1+value_f1},{"00011010",value_c1+value_f1},{"01011000",value_c1+value_f1},\
            {"011100",value_c1},{"001110",value_c1},{"011010",value_c1},{"010110",value_c1},{"111000111",value_c1},{"11001011",value_c1},\
            {"11010011",value_c1},{"1001101",value_c1},{"1010101",value_c1},{"1011001",value_c1},{"\0",0}};         //己方活三或相当于活三
*/

Qixing nqxc2[]={{"0022200",value_c2+value_d2},{"0222000222",value_c2+value_d2},{"2220002220",value_c2+value_d2},{"220020220",value_c2+value_d2},{"022020022",value_c2+value_d2},\
            {"20022020",value_c2+value_d2},{"02022002",value_c2+value_d2},{"00022002022",value_c2+value_e2},{"22020022000",value_c2+value_e2},\
            {"00202020200",value_c2+value_f2*2},{"002020202",value_c2+value_f2},{"202020200",value_c2+value_f2},{"00022020",value_c2+value_f2},{"02022000",value_c2+value_f2},\
            {"022200",value_c2},{"002220",value_c2},{"022020",value_c2},{"020220",value_c2},{"222000222",value_c2},{"22002022",value_c2},\
            {"22020022",value_c2},{"2002202",value_c2},{"2020202",value_c2},{"2022002",value_c2},{"\0",0}};         //对方活三或相当于活三
Qixing nqxd1[]={{"11100",value_d1},{"11010",value_d1},{"11001",value_d1},{"10110",value_d1},{"10101",value_d1},{"01110",value_d1},{"00111",value_d1},\
            {"01011",value_d1},{"10011",value_d1},{"01101",value_d1},{"\0",0}};                                         //己方眠三
Qixing nqxd2[]={{"22200",value_d2},{"22020",value_d2},{"22002",value_d2},{"20220",value_d2},{"20202",value_d2},{"02220",value_d2},{"00222",value_d2},\
            {"02022",value_d2},{"20022",value_d2},{"02202",value_d2},{"\0",0}};                                         //对方眠三
Qixing nqxe1[]={{"00011000",value_e1+value_f1},{"011000",value_e1},{"001100",value_e1},{"000110",value_e1},{"010100",value_e1},{"001010",value_e1},{"010010",value_e1},\
            {"11000011",value_e1},{"1001001",value_e1},{"1000101",value_e1},{"1010001",value_e1},{"\0",0}};         //己方活二或相当于活二                                /*-1000是为了避免重复计数*/
Qixing nqxe2[]={{"00022000",value_e2+value_f2},{"022000",value_e2},{"002200",value_e2},{"000220",value_e2},{"020200",value_e2},{"002020",value_e2},{"020020",value_e2},\
            {"22000022",value_e2},{"2002002",value_e2},{"2000202",value_e2},{"2020002",value_e2},{"\0",0}};         //对方活二或相当于活二                                /*-1000是为了避免重复计数*/
Qixing nqxf1[]={{"11000",value_f1},{"10100",value_f1},{"10010",value_f1},{"10001",value_f1},{"01100",value_f1},{"01010",value_f1},\
            {"01001",value_f1},{"00110",value_f1},{"00101",value_f1},{"00011",value_f1},{"\0",0}};                  //己方眠二
Qixing nqxf2[]={{"22000",value_f2},{"20200",value_f2},{"20020",value_f2},{"20002",value_f2},{"02200",value_f2},{"02020",value_f2},
            {"02002",value_f2},{"00220",value_f2},{"00202",value_f2},{"00022",value_f2},{"\0",0}};                  //对方眠二
Qixing nqxg1[]={{"000000010000000",(float)3.0},{"0000001000000",(float)2.8},{"00000100000",(float)2.5},{"000010000",(float)2.0},{"0001000",(float)1.5},{"000100",(float)1.3},
            {"001000",(float)1.3},{"010000",(float)1.0},{"000010",(float)1.0},{"\0",0}};                            //己方活一
Qixing nqxg2[]={{"000000020000000",(float)3.0},{"0000002000000",(float)2.8},{"00000200000",(float)2.5},{"000020000",(float)2.0},{"0002000",(float)1.5},{"000200",(float)1.3},
            {"002000",(float)1.3},{"020000",(float)1.0},{"000020",(float)1.0},{"\0",0}};                                //对方活一

float boardvalue(Board board[][MAXSIZE+2],int thisstep,bool skip);      //定义在后面
float testpointvalue(Board board[][MAXSIZE+2],int thisstep,int testx,int testy,bool skip);      //定义在后面

//---------------------------棋形判断以及胜负判断------------------------------//
bool checkqx(Board board[][MAXSIZE+2],Qixing qx[],int x,int y,int dir,int kind,int thisstep)
{
    if(qx[kind].value==0) return 0;
    if(board[x][y].whosechess!=0)   return 0;
    int k;
    char c;
    char *p;
    p=strchr(qx[kind].qx,'3');
    for(k=0;k<=p-qx[kind].qx;k++)
    {
        x-=direction[dir][0];
        y-=direction[dir][1];
    }
    for(k=0;k<strlen(qx[kind].qx);k++)
    {
        x+=direction[dir][0];
        y+=direction[dir][1];
        if(!(x>=0&&x<=sizex+1&&y>=0&&y<=sizey+1))return 0;
        if(board[x][y].whosechess==4)c='4';
        else if(board[x][y].whosechess==0)c='0';
        else if(board[x][y].step>0&&((thisstep-board[x][y].whosechess)%2))c='2';
        else if(board[x][y].step>0)c='1';else {printf("c=%c,x=%d,y=%d,board[x][y].whosechess=%d,board[x][y].step=%d,checkqx函数出错\n",c,x,y,board[x][y].whosechess,board[x][y].step);system("pause");}
        if(c=='0'&&qx[kind].qx[k]=='3')continue;
        if(c!=qx[kind].qx[k])return 0;
    }
    return 1;
}
bool checknqx(Board board[][MAXSIZE+2],Qixing nqx[],int x,int y,int ndir,int kind,int attack_or_defence,int thisstep)//attack_or_defence:1.进攻棋形，2.防守棋形
{

    if(nqx[kind].value==0) return 0;
    int k;
    char c;
    char *p;
    p=strchr(nqx[kind].qx,'0'+attack_or_defence);
    for(k=0;k<=p-nqx[kind].qx;k++)
    {
        x-=ndirection[ndir][0];
        y-=ndirection[ndir][1];
    }
    for(k=0;k<strlen(nqx[kind].qx);k++)
    {
        x+=ndirection[ndir][0];
        y+=ndirection[ndir][1];
        if(x<1||x>sizex||y<1||y>sizey)return 0;
        if(board[x][y].whosechess==4)c='4';
        else if(board[x][y].whosechess==0)c='0';
        else if(board[x][y].step>0&&((thisstep-board[x][y].whosechess)%2))c='2';
        else if(board[x][y].step>0)c='1';else {printf("c=%c,x=%d,y=%d,board[x][y].whosechess=%d,board[x][y].step=%d,checknqx函数出错\n",c,x,y,board[x][y].whosechess,board[x][y].step);system("pause");}
        if(c!=nqx[kind].qx[k])return 0;
    }
    //printf("x=%d,y=%d,ndir=%d,qx=%s,p=%s,atorde=%d,thisstep=%d\n",x,y,ndir,nqx[kind].qx,p,attack_or_defence,thisstep);
    return 1;
}

int line(Board realboard[][MAXSIZE+2],int dir,int x,int y,int color)
{
    int i;
    for(i=0;realboard[x+direction[dir][0]][y+direction[dir][1]].step>0&&
        realboard[x+direction[dir][0]][y+direction[dir][1]].whosechess==color;i++)
    {
        x=x+direction[dir][0];
        y=y+direction[dir][1];
        if(!(x>=1&&x<=sizex&&y>=1&&y<=sizey))return i;
    }
    return i;
}
bool win(Board realboard[][MAXSIZE+2],int x,int y,int color)
{
    if(line(realboard,0,x,y,color)+line(realboard,1,x,y,color)>3)
        return true;
    if(line(realboard,2,x,y,color)+line(realboard,3,x,y,color)>3)
        return true;
    if(line(realboard,4,x,y,color)+line(realboard,5,x,y,color)>3)
        return true;
    if(line(realboard,6,x,y,color)+line(realboard,7,x,y,color)>3)
        return true;
    return false;
}
//----------------------棋盘价值判断-----------------------//
bool search_nqx_dir0(Board board[][MAXSIZE+2],Qixing nqx[],int y,int thisstep,int attack_or_defence,float &addvalue)
{
    int dir=0,kind,x;
    bool find=0;
    addvalue=0;
    for(kind=0;nqx[kind].value!=0;kind++)
    {
        for(x=1;x<=sizex-4;x++)
        {
            if(board[x][y].whosechess==0||board[x][y].whosechess!=((thisstep%2 == attack_or_defence%2) ? 1 : 2 ))continue;
            addvalue+=(find=checknqx(board,nqx,x,y,dir,kind,attack_or_defence,thisstep))*nqx[kind].value;
            if(find)break;
        }
        if(find)break;
    }
//  if(find)printf("x=%d,y=%d,board[x][y].whosechess=%d,atotde=%d,thisstep=%d,check=%d\n",x,y,board[x][y].whosechess,attack_or_defence,thisstep,board[x][y].whosechess!=((thisstep%2 == attack_or_defence%2) ? 1 : 2 ));
    return find;
}
bool search_nqx_dir1(Board board[][MAXSIZE+2],Qixing nqx[],int x,int thisstep,int attack_or_defence,float &addvalue)
{
    int dir=1,kind,y;
    bool find=0;
    addvalue=0;
    for(kind=0;nqx[kind].value!=0;kind++)
    {
        for(y=1;y<=sizey-4;y++)
        {
            if(board[x][y].whosechess==0||board[x][y].whosechess!=((thisstep%2 == attack_or_defence%2) ? 1 : 2 ))continue;
            addvalue+=(find=checknqx(board,nqx,x,y,dir,kind,attack_or_defence,thisstep))*nqx[kind].value;
            if(find)break;
        }
        if(find)break;
    }
    return find;
}
bool search_nqx_dir2(Board board[][MAXSIZE+2],Qixing nqx[],int initx,int inity,int thisstep,int attack_or_defence,float &addvalue)
{
    int dir=2,kind,x,y;
    bool find=0;
    addvalue=0;
    for(kind=0;nqx[kind].value!=0;kind++)
    {
        x=initx;    y=inity;
        while(x<=sizex-4&&y<=sizey-4)
        {
            if(board[x][y].whosechess==0||board[x][y].whosechess!=((thisstep%2 == attack_or_defence%2) ? 1 : 2 )){  x+=1;   y+=1;   continue;}
            addvalue+=(find=checknqx(board,nqx,x,y,dir,kind,attack_or_defence,thisstep))*nqx[kind].value;
            if(find)break;
            x+=1;   y+=1;
        }
        if(find)break;
    }
    return find;
}
bool search_nqx_dir3(Board board[][MAXSIZE+2],Qixing nqx[],int initx,int inity,int thisstep,int attack_or_defence,float &addvalue)
{
    int dir=3,kind,x,y;
    bool find=0;
    addvalue=0;
    for(kind=0;nqx[kind].value!=0;kind++)
    {
        x=initx;    y=inity;
        while(x<=sizex-4&&y>=5)
        {
            if(board[x][y].whosechess==0||board[x][y].whosechess!=((thisstep%2 == attack_or_defence%2) ? 1 : 2 )){  x+=1;   y-=1;   continue;}
            addvalue+=(find=checknqx(board,nqx,x,y,dir,kind,attack_or_defence,thisstep))*nqx[kind].value;
            if(find)break;
            x+=1;   y-=1;
        }
        if(find)break;
    }
    return find;
}
void count_nqx1(Board board[][MAXSIZE+2],int thisstep,float &value,int &nd1,int &ne1,int &nf1,int &ng1) //己方棋形统计，及初步value估计
{
    int dir=0,kind=0,x,y;
    bool find=0;
    float addvalue;
    value=0.0;nd1=0;ne1=0;nf1=0;ng1=0;
    //dir=0,水平方向
    for(y=1;y<=sizey;y++)
    {
        find=0;
        find=search_nqx_dir0(board,nqxd1,y,thisstep,1,addvalue);
        if(find){value+=addvalue;nd1++;continue;}
        find=search_nqx_dir0(board,nqxe1,y,thisstep,1,addvalue);
        if(find){value+=addvalue;ne1++;continue;}
        find=search_nqx_dir0(board,nqxf1,y,thisstep,1,addvalue);
        if(find){value+=addvalue;nf1++;continue;}
        find=search_nqx_dir0(board,nqxg1,y,thisstep,1,addvalue);
        if(find){value+=addvalue;ng1++;continue;}
    }
    //dir=1,竖直方向
    for(x=1;x<=sizex;x++)
    {
        find=0;
        find=search_nqx_dir1(board,nqxd1,x,thisstep,1,addvalue);
        if(find){value+=addvalue;nd1++;continue;}
        find=search_nqx_dir1(board,nqxe1,x,thisstep,1,addvalue);
        if(find){value+=addvalue;ne1++;continue;}
        find=search_nqx_dir1(board,nqxf1,x,thisstep,1,addvalue);
        if(find){value+=addvalue;nf1++;continue;}
        find=search_nqx_dir1(board,nqxg1,x,thisstep,1,addvalue);
        if(find){value+=addvalue;ng1++;continue;}
    }
    //dir=2,斜方向（1,1）
    for(y=sizey-4;y>1;y--)
    {
        x=1;    find=0;
        find=search_nqx_dir2(board,nqxd1,x,y,thisstep,1,addvalue);
        if(find){value+=addvalue;nd1++;continue;}
        find=search_nqx_dir2(board,nqxe1,x,y,thisstep,1,addvalue);
        if(find){value+=addvalue;ne1++;continue;}
        find=search_nqx_dir2(board,nqxf1,x,y,thisstep,1,addvalue);
        if(find){value+=addvalue;nf1++;continue;}
        find=search_nqx_dir2(board,nqxg1,x,y,thisstep,1,addvalue);
        if(find){value+=addvalue;ng1++;continue;}
    }
    for(x=1;x<=sizex-4;x++)
    {
        y=1;    find=0;
        find=search_nqx_dir2(board,nqxd1,x,y,thisstep,1,addvalue);
        if(find){value+=addvalue;nd1++;continue;}
        find=search_nqx_dir2(board,nqxe1,x,y,thisstep,1,addvalue);
        if(find){value+=addvalue;ne1++;continue;}
        find=search_nqx_dir2(board,nqxf1,x,y,thisstep,1,addvalue);
        if(find){value+=addvalue;nf1++;continue;}
        find=search_nqx_dir2(board,nqxg1,x,y,thisstep,1,addvalue);
        if(find){value+=addvalue;ng1++;continue;}
    }
    //dir=3,斜方向（1,-1）
    for(y=5;y<sizey;y++)
    {
        x=1;    find=0;
        find=search_nqx_dir3(board,nqxd1,x,y,thisstep,1,addvalue);
        if(find){value+=addvalue;nd1++;continue;}
        find=search_nqx_dir3(board,nqxe1,x,y,thisstep,1,addvalue);
        if(find){value+=addvalue;ne1++;continue;}
        find=search_nqx_dir3(board,nqxf1,x,y,thisstep,1,addvalue);
        if(find){value+=addvalue;nf1++;continue;}
        find=search_nqx_dir3(board,nqxg1,x,y,thisstep,1,addvalue);
        if(find){value+=addvalue;ng1++;continue;}
    }
    for(x=1;x<=sizex-4;x++)
    {
        y=sizey;    find=0;
        find=search_nqx_dir3(board,nqxd1,x,y,thisstep,1,addvalue);
        if(find){value+=addvalue;nd1++;continue;}
        find=search_nqx_dir3(board,nqxe1,x,y,thisstep,1,addvalue);
        if(find){value+=addvalue;ne1++;continue;}
        find=search_nqx_dir3(board,nqxf1,x,y,thisstep,1,addvalue);
        if(find){value+=addvalue;nf1++;continue;}
        find=search_nqx_dir3(board,nqxg1,x,y,thisstep,1,addvalue);
        if(find){value+=addvalue;ng1++;continue;}
    }
}

void count_nqx2(Board board[][MAXSIZE+2],int thisstep,float &value,int &nc2,int &nd2,int &ne2,int &nf2,int &ng2)    //对方棋形统计，及初步value估计
{
    int dir=0,kind=0,x,y;
    bool find=0;
    float addvalue;
    value=0.0;nc2=0;nd2=0;ne2=0;nf2=0;ng2=0;
    //dir=0,水平方向
    for(y=1;y<=sizey;y++)
    {
        find=0;
        find=search_nqx_dir0(board,nqxc2,y,thisstep,2,addvalue);
        if(find){value+=addvalue;nc2++;continue;}
        find=search_nqx_dir0(board,nqxd2,y,thisstep,2,addvalue);
        if(find){value+=addvalue;nd2++;continue;}
        find=search_nqx_dir0(board,nqxe2,y,thisstep,2,addvalue);
        if(find){value+=addvalue;ne2++;continue;}
        find=search_nqx_dir0(board,nqxf2,y,thisstep,2,addvalue);
        if(find){value+=addvalue;nf2++;continue;}
        find=search_nqx_dir0(board,nqxg2,y,thisstep,2,addvalue);
        if(find){value+=addvalue;ng2++;continue;}
    }
    //dir=1,竖直方向
    for(x=1;x<=sizex;x++)
    {
        find=0;
        find=search_nqx_dir1(board,nqxc2,x,thisstep,2,addvalue);
        if(find){value+=addvalue;nc2++;continue;}
        find=search_nqx_dir1(board,nqxd2,x,thisstep,2,addvalue);
        if(find){value+=addvalue;nd2++;continue;}
        find=search_nqx_dir1(board,nqxe2,x,thisstep,2,addvalue);
        if(find){value+=addvalue;ne2++;continue;}
        find=search_nqx_dir1(board,nqxf2,x,thisstep,2,addvalue);
        if(find){value+=addvalue;nf2++;continue;}
        find=search_nqx_dir1(board,nqxg2,x,thisstep,2,addvalue);
        if(find){value+=addvalue;ng2++;continue;}
    }
    //dir=2,斜方向（1,1）
    for(y=sizey-4;y>1;y--)
    {
        x=1;    find=0;
        find=search_nqx_dir2(board,nqxc2,x,y,thisstep,2,addvalue);
        if(find){value+=addvalue;nc2++;continue;}
        find=search_nqx_dir2(board,nqxd2,x,y,thisstep,2,addvalue);
        if(find){value+=addvalue;nd2++;continue;}
        find=search_nqx_dir2(board,nqxe2,x,y,thisstep,2,addvalue);
        if(find){value+=addvalue;ne2++;continue;}
        find=search_nqx_dir2(board,nqxf2,x,y,thisstep,2,addvalue);
        if(find){value+=addvalue;nf2++;continue;}
        find=search_nqx_dir2(board,nqxg2,x,y,thisstep,2,addvalue);
        if(find){value+=addvalue;ng2++;continue;}
    }
    for(x=1;x<=sizex-4;x++)
    {
        y=1;    find=0;
        find=search_nqx_dir2(board,nqxc2,x,y,thisstep,2,addvalue);
        if(find){value+=addvalue;nc2++;continue;}
        find=search_nqx_dir2(board,nqxd2,x,y,thisstep,2,addvalue);
        if(find){value+=addvalue;nd2++;continue;}
        find=search_nqx_dir2(board,nqxe2,x,y,thisstep,2,addvalue);
        if(find){value+=addvalue;ne2++;continue;}
        find=search_nqx_dir2(board,nqxf2,x,y,thisstep,2,addvalue);
        if(find){value+=addvalue;nf2++;continue;}
        find=search_nqx_dir2(board,nqxg2,x,y,thisstep,2,addvalue);
        if(find){value+=addvalue;ng2++;continue;}
    }
    //dir=3,斜方向（1,-1）
    for(y=5;y<sizey;y++)
    {
        x=1;    find=0;
        find=search_nqx_dir3(board,nqxc2,x,y,thisstep,2,addvalue);
        if(find){value+=addvalue;nc2++;continue;}
        find=search_nqx_dir3(board,nqxd2,x,y,thisstep,2,addvalue);
        if(find){value+=addvalue;nd2++;continue;}
        find=search_nqx_dir3(board,nqxe2,x,y,thisstep,2,addvalue);
        if(find){value+=addvalue;ne2++;continue;}
        find=search_nqx_dir3(board,nqxf2,x,y,thisstep,2,addvalue);
        if(find){value+=addvalue;nf2++;continue;}
        find=search_nqx_dir3(board,nqxg2,x,y,thisstep,2,addvalue);
        if(find){value+=addvalue;ng2++;continue;}
    }
    for(x=1;x<=sizex-4;x++)
    {
        y=sizey;    find=0;
        find=search_nqx_dir3(board,nqxc2,x,y,thisstep,2,addvalue);
        if(find){value+=addvalue;nc2++;continue;}
        find=search_nqx_dir3(board,nqxd2,x,y,thisstep,2,addvalue);
        if(find){value+=addvalue;nd2++;continue;}
        find=search_nqx_dir3(board,nqxe2,x,y,thisstep,2,addvalue);
        if(find){value+=addvalue;ne2++;continue;}
        find=search_nqx_dir3(board,nqxf2,x,y,thisstep,2,addvalue);
        if(find){value+=addvalue;nf2++;continue;}
        find=search_nqx_dir3(board,nqxg2,x,y,thisstep,2,addvalue);
        if(find){value+=addvalue;ng2++;continue;}
    }
}
int value_of_a_qx(Board board[][MAXSIZE+2],Qixing qx[],int x,int y,int thisstep)
{
    if(board[x][y].whosechess)  return 0;
    bool find=0;
    int dir=0,kind=0,value=0;   //  printf("%s",qx[kind].qx);printf("%d:",qx[kind].value);
    for(dir=0;dir<=7;dir++)
    {
        find=0;
        for(kind=0;qx[kind].value!=0;kind++)
        {
            value+=(find=checkqx(board,qx,x,y,dir,kind,thisstep))*qx[kind].value;
            if(find)break;
        }
        dir++;
        if(find)continue;
        for(kind=0;qx[kind].value!=0;kind++)
        {
            value+=(find=checkqx(board,qx,x,y,dir,kind,thisstep))*qx[kind].value;
            if(find)break;
        }
    }
    return value;
}

int pointvalue(Board board[][MAXSIZE+2],int x,int y,int thisstep,float attack)      /*从c1类开始求价值之和*/
{
    if(board[x][y].whosechess)  return -3000;
    int value=0;
    value=value+value_of_a_qx(board,qxc1,x,y,thisstep)*attack+value_of_a_qx(board,qxd2,x,y,thisstep)*attack+value_of_a_qx(board,qxd1,x,y,thisstep)*attack+value_of_a_qx(board,qxe1,x,y,thisstep)*attack+value_of_a_qx(board,qxf1,x,y,thisstep)*attack+value_of_a_qx(board,qxe2,x,y,thisstep)*attack+value_of_a_qx(board,qxf2,x,y,thisstep)+value_of_a_qx(board,qxg2,x,y,thisstep);
    if((x==1&&y==1)||(x==1&&y==sizey)||(x==sizex&&y==1)||(x==sizex&&y==sizey))  value-=10;
    else if(x+y==3||sizex+1-x+y==3||sizey+1+x-y==3||sizex+sizey+2-x-y==3)   value-=9;
    else if(x+y==4||sizex+1-x+y==4||sizey+1+x-y==4||sizex+sizey+2-x-y==4)   value-=8;
    else if(x==1||x==sizex||y==1||y==sizey) value-=7;
    else if(x==2||x==sizex-1||y==2||y==sizey-1) value-=6;
    else if(x==3||x==sizex-2||y==3||y==sizey-2) value-=5;
    else if(x==4||x==sizex-3||y==4||y==sizey-3) value-=4;
    else if(x==5||x==sizex-4||y==5||y==sizey-4) value-=3;
    else if(x==6||x==sizex-5||y==6||y==sizey-5) value-=2;
    else if(x==7||x==sizex-6||y==7||y==sizey-6)     value-=1;
    return value;
}

bool checkif_sisi_sisan(Board board[][MAXSIZE+2],int x,int y,int thisstep)
{
    if(board[x][y].whosechess)  return 0;
    bool find=0,havesi=0,answer=0;
    int dir=0,kind=0,value=0;   //  printf("%s",qx[kind].qx);printf("%d:",qx[kind].value);
    for(dir=0;dir<=7;dir++)
    {
        find=0;
        for(kind=0;qxc1[kind].value!=0;kind++)
        {
            value+=(find=checkqx(board,qxc1,x,y,dir,kind,thisstep))*qxc1[kind].value;
            if(find){havesi=1;break;}
        }
        dir++;
        if(find)continue;
        for(kind=0;qxc1[kind].value!=0;kind++)
        {
            value+=(find=checkqx(board,qxc1,x,y,dir,kind,thisstep))*qxc1[kind].value;
            if(find){havesi=1;break;}
        }
        if(find)continue;
        for(kind=0;qxd1[kind].value!=0;kind++)
        {
            value+=(find=checkqx(board,qxd1,x,y,dir,kind,thisstep))*qxd1[kind].value;
            if(find){break;}
        }
        if(find)continue;
        dir--;
        for(kind=0;qxd1[kind].value!=0;kind++)
        {
            value+=(find=checkqx(board,qxd1,x,y,dir,kind,thisstep))*qxd1[kind].value;
            if(find){break;}
        }
        dir++;
    }
    answer=havesi&&value>=2000;
//printf("check4:x=%d,y=%d,havesi=%d,value=%d\n",x,y,havesi,value);
    return answer;
}

bool checkif_sansan(Board board[][MAXSIZE+2],int x,int y,int thisstep)
{
    if(board[x][y].whosechess)  return 0;
    bool find=0,answer=0;
    int dir=0,kind=0,value=0;   //  printf("%s",qx[kind].qx);printf("%d:",qx[kind].value);
    for(dir=0;dir<=7;dir++)
    {
        find=0;
        for(kind=0;qxd1[kind].value!=0;kind++)
        {
            value+=(find=checkqx(board,qxd1,x,y,dir,kind,thisstep))*qxd1[kind].value;
            if(find){break;}
        }
        dir++;
        if(find)continue;
        for(kind=0;qxd1[kind].value!=0;kind++)
        {
            value+=(find=checkqx(board,qxd1,x,y,dir,kind,thisstep))*qxd1[kind].value;
            if(find){break;}
        }
    }
    answer=value>=2000;
//printf("check3:x=%d,y=%d,value=%d\n",x,y,value);
    return answer;
}

bool best_chongsi(Board board[][MAXSIZE+2],int &num,int *bestx,int *besty,int thisstep,bool ifblock)
{
    bool find=0;
    int i,x,y,bestvalue[n_VCF_MAX],tempvalue,findnum=0,cvalue,cx,cy;
    for(i=0;i<num;i++){bestx[i]=-1;besty[i]=-1;bestvalue[i]=-111111;}
    for(x=1;x<=sizex;x++)  for(y=1;y<=sizey;y++)
    {
        if(board[x][y].abandon||board[x][y].whosechess||(ifblock&&board[x][y].block))continue;
        if(value_of_a_qx(board,qxc1,x,y,thisstep))
        {
            findnum++;find=1;
            tempvalue=pointvalue(board,x,y,thisstep,10.0);//主要考虑进攻value
            if(bestvalue[num-1]==-111111||tempvalue>bestvalue[num-1])
            {
                bestvalue[num-1]=tempvalue;bestx[num-1]=x;besty[num-1]=y;
                for(i=num-1;i>=1;i--)
                {
                    if(bestvalue[i]>bestvalue[i-1])
                    {
                        cvalue=bestvalue[i];cx=bestx[i];cy=besty[i];
                        bestvalue[i]=bestvalue[i-1];bestx[i]=bestx[i-1];besty[i]=besty[i-1];
                        bestvalue[i-1]=cvalue;bestx[i-1]=cx;besty[i-1]=cy;
                    }
                    else break;
                }
            }
        }
    }
    num=min(num,findnum);
    return find;
}

void clean_temp_change(Board board[][MAXSIZE+2],int thisstep)
{
    int x,y;
    for(x=1;x<=sizex;x++)  for(y=1;y<=sizey;y++)
    {
        if(board[x][y].step>=thisstep)
        {
            board[x][y].step=0;
            board[x][y].whosechess=0;
        }
    }
}



//---------------------------------搜索树与对称性分析---------------------------------------//
struct node
{
    node *last;
    node *next;
    node *right_brother;
    node *parent;
    char x;     //坐标值
    char y;
    short nodepiecenum;//该点是第几步
    float value;
    bool all_son;   //是否已连接所有可能的下一步
    long int n_children;
    long int noloseblack;
    long int nolosewhite;
};
struct node *p_start=NULL;
struct node *p_real=NULL;
struct node *p_now=NULL;
struct node *p_new=NULL;

void write_all_node()
{
    node *ptemp=p_start;
    for(ptemp=p_start;ptemp!=NULL;ptemp=ptemp->next)
    {
        printf("第%d步 x=%d,y=%d,value=%f\n",ptemp->nodepiecenum,ptemp->x,ptemp->y,ptemp->value);
    }
}
void add_new_node(node *p_parent,int x,int y,int thisstep,float value)
{
    node *ptemp=p_parent;
    p_new=NULL;
    if(p_parent!=NULL && p_parent->nodepiecenum+1!=thisstep)
    {printf("ERROR1 in add_new_node: p_parent->nodepiecenum=%d,thisstep=%d\n",p_parent->nodepiecenum,thisstep);system("pause");return;}
    p_new = (struct node*)malloc(sizeof(struct node));
    p_new->x=x;p_new->y=y;p_new->nodepiecenum=thisstep;p_new->value=value;p_new->all_son=0;
    p_new->n_children=0;p_new->noloseblack=0;p_new->nolosewhite=0;
    p_new->parent=p_parent;
    if(p_parent==NULL)
    {
        if(thisstep>0){printf("ERROR2 in add_new_node\n");system("pause");return;}
        //第一个node (nodepiecenum=0)(p_start)
        p_new->last=NULL;
        p_new->next=NULL;
        p_new->right_brother=NULL;
        p_start=p_new;
    }
    else if(p_parent->next==NULL || p_parent->next->nodepiecenum<=p_parent->nodepiecenum)   //无儿子
    {
        p_new->last=p_new->parent;
        p_new->next=p_new->parent->next;
        p_new->right_brother=NULL;
        if(p_new->last!=NULL)   p_new->last->next=p_new;
        if(p_new->next!=NULL)   p_new->next->last=p_new;
    }
    else    //有儿子
    {
        while(ptemp->next!=NULL&&ptemp->next->nodepiecenum>p_parent->nodepiecenum)  ptemp=ptemp->next;
        p_new->last=ptemp;
        p_new->next=ptemp->next;
        p_new->right_brother=NULL;
        if(p_new->last!=NULL)   p_new->last->next=p_new;
        if(p_new->next!=NULL)   p_new->next->last=p_new;
        ptemp=p_parent->next;
        while(ptemp->right_brother!=NULL&&ptemp->right_brother!=p_new)  ptemp=ptemp->right_brother;
        ptemp->right_brother=p_new;
    }
    total_node++;
    for(ptemp=p_parent;ptemp!=NULL;ptemp=ptemp->parent) ptemp->n_children++;
//  printf("add_value=%f,%f\n",p_new->value,value);system("pause");
}


void get_symmetry_xy(int x2,int y2,int &x,int &y,int type)
{
    switch(type)
    {
    case 0: x=x2;           y=y2;           break;  //不变
    case 1: x=x2;           y=sizey+1-y2;   break;  //上下翻转对称
    case 2: x=sizex+1-x2;   y=y2;           break;  //左右
    case 3: x=sizey+1-y2;   y=sizex+1-x2;   break;  //左上、右下
    case 4: x=y2;           y=x2;           break;  //右上、左下
    case 5: x=sizex+1-x2;   y=sizey+1-y2;   break;  //旋转180度
    case 6: x=sizey+1-y2;   y=x2;           break;  //旋转90度
    case 7: x=y2;           y=sizex+1-x2;   break;  //另一方向旋转90度
    }
}


void printboard_num(Board realboard[][MAXSIZE+2]);

bool check_symmetry(Board board[][MAXSIZE+2],int type)
{
    int x,y,x2,y2;
    for(x2=1;x2<=sizex;x2++)
    {
        for(y2=1;y2<=sizey;y2++)
        {
            if(board[x2][y2].whosechess==0)continue;
            get_symmetry_xy(x2,y2,x,y,type);
            if(board[x][y].whosechess!=board[x2][y2].whosechess)
            {
                return 0;
            }
        }
    }
    return 1;
}

bool check_symmetry_and_change(node *p_parent,Board board[][MAXSIZE+2],int type)
{
    int x,y;
    bool check=1;
    check=check_symmetry(board,type);
    if(check==0)    return 0;
    //经检验，确认对称性的同型，则自动修改node坐标值（使之与当前棋盘一致）
    node *ptemp;
    if(p_parent->next!=NULL && p_parent->next->nodepiecenum<=p_parent->nodepiecenum){printf("ERROR1 in check_symmetry_and_change:%d,%d\n",p_parent->next->nodepiecenum,p_parent->nodepiecenum);}
    for(ptemp=p_parent->next;ptemp!=NULL&&ptemp->nodepiecenum>p_parent->nodepiecenum;ptemp=ptemp->next)
    {
        get_symmetry_xy(ptemp->x,ptemp->y,x,y,type);
        ptemp->x=x; ptemp->y=y;
    }
    return 1;
}


void get_board_symmetry(Board board[][MAXSIZE+2],bool board_symmetry[8])
{
    int type;
    bool have_symmetry=0;
    for(type=1;type<=7;type++)
    {
        if(board_symmetry[type]=check_symmetry(board,type)) have_symmetry=1;
    }
    board_symmetry[0]=have_symmetry;
}

void random_symmetry(Board board[][MAXSIZE+2],bool board_symmetry[8],int &move_x,int &move_y)
{
    int type,num,x[8],y[8],tempx,tempy,i,ans;
    bool check;
    if(board_symmetry[0]==0)return;
    num=1; x[0]=move_x; y[0]=move_y;
    for(type=1;type<=7;type++)
    {
        if(board_symmetry[type])
        {
            get_symmetry_xy(move_x,move_y,tempx,tempy,type);
            if(board[tempx][tempy].block)   continue;
            check=1;
            for(i=0;i<num;i++)
            {
                if(x[i]==tempx&&y[i]==tempy){check=0;break;}
            }
            if(check){x[num]=tempx;y[num]=tempy;num++;}
        }
    }
    ans=int(random()*num);
    move_x=x[ans];  move_y=y[ans];
}

bool add_or_find_node(node *p_parent,int x,int y,int totalstep)
{
    bool find=0;
    node *ptemp=NULL;
    int sym_type=0,x2,y2,i,j;
    //printf("%d,%d\n",p_parent==NULL,p_parent->next==NULL);
    //if(p_parent->next!=NULL)printf("%d,%d\n",p_parent->next->nodepiecenum,totalstep);
    if(p_parent==NULL||p_parent->next==NULL||p_parent->next->nodepiecenum<totalstep)
    {
        add_new_node(p_parent,x,y,totalstep,value_UNCOMPUTED);
        return 0;
    }
    if(p_parent->next->nodepiecenum!=totalstep){printf("ERROR1 in check_node_saved:next->nodepiecenum=%d, totalstep=%d\n",p_parent->next->nodepiecenum,totalstep);system("pause");return 0;}
    for(ptemp=p_parent->next;ptemp!=NULL;ptemp=ptemp->right_brother)    if(ptemp->x==x&&ptemp->y==y){p_new=ptemp;   return 1;}
    Board tempboard[MAXSIZE+2][MAXSIZE+2];
    for(i=1;i<=sizex;i++) for(j=1;j<=sizey;j++)
    {
        tempboard[i][j].whosechess=realboard[i][j].whosechess;
    }
    tempboard[x][y].whosechess=0;
    for(ptemp=p_parent;ptemp->nodepiecenum>step-1;ptemp=ptemp->parent)
    {
        tempboard[ptemp->x][ptemp->y].whosechess=2-ptemp->nodepiecenum%2;
    }

    for(ptemp=p_parent->next;ptemp!=NULL;ptemp=ptemp->right_brother)
    {
        x2=ptemp->x;    y2=ptemp->y;
//      printf("%d %d,%d %d\n",x,y,x2,y2);
        if  (                   x==x2           && y==sizey+1-y2)   {if(check_symmetry_and_change(p_parent,tempboard,1)){p_new=ptemp;   return 1;}} //上下翻转对称
        if  (                   x==sizex+1-x2   && y==y2)           {if(check_symmetry_and_change(p_parent,tempboard,2)){p_new=ptemp;   return 1;}} //左右
        if  (sizex==sizey &&    x==sizey+1-y2   && y==sizex+1-x2)   {if(check_symmetry_and_change(p_parent,tempboard,3)){p_new=ptemp;   return 1;}} //左上、右下
        if  (sizex==sizey &&    x==y2           && y==x2)           {if(check_symmetry_and_change(p_parent,tempboard,4)){p_new=ptemp;   return 1;}} //右上、左下
        if  (                   x==sizex+1-x2   && y==sizey+1-y2)   {if(check_symmetry_and_change(p_parent,tempboard,5)){p_new=ptemp;   return 1;}} //旋转180度
        if  (sizex==sizey &&    x==sizey+1-y2   && y==x2)           {if(check_symmetry_and_change(p_parent,tempboard,6)){p_new=ptemp;   return 1;}} //旋转90度
        if  (sizex==sizey &&    x==y2           && y==sizex+1-x2)   {if(check_symmetry_and_change(p_parent,tempboard,7)){p_new=ptemp;   return 1;}} //另一方向旋转90度
    }
    add_new_node(p_parent,x,y,totalstep,value_UNCOMPUTED);
    return 0;
}

void add_node_information(float value)//直接在p_new添加信息
{
//  printf("p_new->value=%f,value=%f\n",p_new->value,value);
    if(p_new->value==value_UNCOMPUTED)  p_new->value=value;
//  printf("p_new->x=%d,p_new->y=%d,p_new->value=%f,value=%f\n",p_new->x,p_new->y,p_new->value,value);
//  printf("p_new->parent->y=%d,p_new->parent->y=%d\n",p_new->x,p_new->parent->x,p_new->parent->y);
    if(p_new->noloseblack==0 && p_new->nolosewhite==0)
    {
        if(value==value_UNCOMPUTED);
        else if(value>=value_win)
        {
            if(step%2)  {p_new->nolosewhite=0;p_new->noloseblack=1;}
            else        {p_new->nolosewhite=1;p_new->noloseblack=0;}
        }
        else if(value<=-value_win)
        {
            if(step%2)  {p_new->nolosewhite=1;p_new->noloseblack=0;}
            else        {p_new->nolosewhite=0;p_new->noloseblack=1;}
        }
        else
        {
            p_new->nolosewhite=1;p_new->noloseblack=1;
        }
    }
}


void forget_all_node()
{
    node *ptemp;
    for(ptemp=p_start;ptemp!=NULL;)
    {
        if(ptemp->next!=NULL)
        {
            ptemp=ptemp->next;
            free(ptemp->last);
        }
        else
        {
            free(ptemp);
            break;
        }
    }
}

void cut_branch(node *p_parent,int num)
{
    node *ptemp,*pdelete,*pnext;
    int i;
    float bestvalue[MAX_KEEP_UPBOUND],tempvalue,cvalue,cutvalue;
//printf("\n cut_branch::");
    for(i=0;i<num;i++)  bestvalue[i]=-111111;
//printf("a");
    for(ptemp=p_parent->next;ptemp!=NULL;ptemp=ptemp->right_brother)
    {
//printf("b");
        tempvalue=ptemp->value;
        if(tempvalue>bestvalue[num-1])
        {
//printf("c");
            bestvalue[num-1]=tempvalue;
            for(i=num-1;i>=1;i--)
            {
                if(bestvalue[i]>bestvalue[i-1])
                {
                    cvalue=bestvalue[i];
                    bestvalue[i]=bestvalue[i-1];
                    bestvalue[i-1]=cvalue;
                }
                else break;
            }
//printf("d");
        }
//printf("D");
    }
//printf("E");
    cutvalue=max(bestvalue[num-1],bestvalue[0]-max_gap);
    if(cutvalue==-111111)return;
//printf("e");
    for(ptemp=p_parent->next;ptemp!=NULL;ptemp=ptemp->right_brother)
    {
//printf("f");
        while(ptemp->value<cutvalue)
        {
//printf("g");
            pdelete=ptemp;
            for(ptemp=pdelete->next;ptemp!=NULL&&ptemp->nodepiecenum>p_parent->next->nodepiecenum;ptemp=ptemp->next);
            if(ptemp!=NULL)pnext=ptemp; else pnext=NULL;
            pdelete->last->next=pnext;
            if(pnext!=NULL)pnext->last=pdelete->last;
            ptemp=p_parent->next;
            while(ptemp->right_brother!=NULL&&ptemp->right_brother!=pdelete)    ptemp=ptemp->right_brother;
            if(ptemp->right_brother==pdelete)ptemp->right_brother=pdelete->right_brother;
//printf("h");
            for(ptemp=pdelete->next;ptemp!=NULL&&ptemp->nodepiecenum>p_parent->next->nodepiecenum;ptemp=ptemp->next)
            {
                free(ptemp->last);
                if(ptemp->next!=NULL&&ptemp->next->nodepiecenum<=p_parent->next->nodepiecenum){free(ptemp);break;}
            }
            ptemp=p_parent->next;//重新定位
//printf("i");
        }
//printf("j");
    }
//printf("k\n");
//system("pause");
//write_all_node();
//system("pause");
}
//----------------------------------获取棋子位置信息--------------------------------//
int getx(Board board[][MAXSIZE+2],int whichstep)
{
    int i,j;
    for(i=1;i<=sizex;i++)
        for(j=1;j<=sizey;j++)
            if(board[i][j].step==whichstep)
            {//printf("x=%d",i);
                return i;
            }
    printf("程序错误！未找到指定步数的落子位置。\n");
    system("pause");
    return 0;
}
int gety(Board board[][MAXSIZE+2],int whichstep)
{
    int i,j;
    for(i=1;i<=sizex;i++)
        for(j=1;j<=sizey;j++)
            if(board[i][j].step==whichstep)
            {
            //  printf("y=%d",j);
                return j;
            }
    printf("程序错误！未找到指定步数的落子位置。\n");
    system("pause");
    return 0;
}
//---------------------------------------输出棋盘数据-----------------------------------------//
void printboard_num(Board realboard[][MAXSIZE+2])
{
    int i,j;
    printf("  ");
    for(j=1;j<=sizey;j++)   printf("%2d",j);printf("\n");
    for(i=1;i<=sizey;i++)
    {
        printf("%2d",i);
        for(j=1;j<=sizex;j++)
        {if(realboard[j][i].whosechess==1)  printf("○");
        else if(realboard[j][i].whosechess==2)  printf("●");
        else if(realboard[j][i].whosechess==0)  printf("┼");
        else printf("%2d",realboard[j][i].whosechess);
        }
        printf("%2d",i);
        printf("\n");
    }
    printf("  ");
    for(j=1;j<=sizey;j++)   printf("%2d",j);printf("\n");
}
void printboard_step(Board realboard[][MAXSIZE+2])
{
    int i,j;
    for(i=0;i<=sizey+1;i++)
    {
        for(j=1;j<=sizex;j++)
        {if(realboard[j][i].step)printf("%4d",realboard[j][i].step);else printf("   .");}
        printf("\n\n");
    }
}
void printboard_value(Board realboard[][MAXSIZE+2])
{
    int i,j;
    for(i=1;i<=sizey;i++)
    {
        for(j=1;j<=sizex;j++)
            if(realboard[j][i].whosechess==1)   printf(" ○ ");
            else if(realboard[j][i].whosechess==2)  printf(" ● ");
            else if(realboard[j][i].value==value_ABANDON)   printf(" *  ");
            else if(realboard[j][i].value>=value_win)   printf("胜%2d",sizex*sizey-(realboard[j][i].value-value_win));
            else if(realboard[j][i].value<=-value_win)  printf("败%2d",sizex*sizey-(abs(realboard[j][i].value)-value_win));
            else printf("%4.0f",realboard[j][i].value);
        printf("\n\n");
    }
}
//--------------------------其他函数-------------------------------//


void restart(int width,int height)                  /*初始棋局*/
{
    int i,j;
    if(width!=sizex||height!=sizey)//更改棋盘大小
    {
        forget_all_node();
        sizex=width;sizey=height;
    }
    p_real=NULL;
    for(i=0;i<=sizex+1;i++)
        for(j=0;j<=sizey+1;j++)
        {
            if(i==0||i==sizex+1||j==0||j==sizey+1)
                realboard[i][j].whosechess=4;
            else
                realboard[i][j].whosechess=0;
            realboard[i][j].step=0;
            realboard[i][j].value=0;
            realboard[i][j].block=0;
            realboard[i][j].abandon=0;
        }
    step=1;
    srand((unsigned)time(NULL));rand();
    timeout_left=timeout_match;
    n_AI=0;
    if(p_start==NULL)add_new_node(p_real,-1,-1,0,value_UNCOMPUTED);
    p_real=p_start;
}
/*
void copyboard(Board lastboard[][MAXSIZE+2],Board tempboard[][MAXSIZE+2])
{                               //将真实棋盘数据复制到临时分析用的棋盘,或上一分析棋盘复制到下一分析棋盘
    int i,j;
    for(i=0;i<=sizex+1;i++)
        for(j=0;j<=sizey+1;j++)
        {
            tempboard[i][j].value=0;
            tempboard[i][j].abandon=0;
            tempboard[i][j].step=lastboard[i][j].step;
            tempboard[i][j].whosechess=lastboard[i][j].whosechess;
            tempboard[i][j].block=lastboard[i][j].block;
        }
}
*/

//--------------------------几个常用函数-------------------------------//
bool check_timeout()
{
    bool check=0;
//  printf("timeout_turn=%d,timeout_left/timeout_num=%d,timeout_gap=%d\n",timeout_turn,timeout_left/timeout_num,timeout_gap);
//  printf("min1=%d,min2=%d\n",min(timeout_turn,timeout_left/timeout_num),min(timeout_turn-timeout_gap,timeout_left/timeout_num-timeout_gap));
//  printf("Get=%d,old=%d,elapse=%d\n",GetTickCount(),oldtime,GetTickCount()-oldtime);
    check=(min(timeout_turn-timeout_gap_real,(timeout_left-timeout_gap_real)/timeout_num-timeout_gap)<=(GetTickCount()-oldtime));
//  check=(min(timeout_turn,timeout_left/timeout_num)-(GetTickCount()-oldtime)<=timeout_gap);
    if(check) quickanswer=1;
    return check;
}


void cal_abandon(Board board[][MAXSIZE+2])
{
    int x,y,dx,dy;
    for(x=1;x<=sizex;x++) for(y=1;y<=sizey;y++) board[x][y].abandon=1;

    for(x=1;x<=sizex;x++)  for(y=1;y<=sizey;y++)
    {
        if(board[x][y].whosechess==1||board[x][y].whosechess==2)
        {
            for(dx=-2;dx<=2;dx++) for(dy=-2;dy<=2;dy++)
            {
                if(x+dx<1||x+dx>sizex||y+dy<1||y+dy>sizey)continue;
                board[x+dx][y+dy].abandon=0;
            }
            if(x-3>=1)board[x-3][y].abandon=0;
            if(x+3<=sizex)board[x+3][y].abandon=0;
            if(y-3>=1)board[x][y-3].abandon=0;
            if(y+3<=sizey)board[x][y+3].abandon=0;
            if(x-3>=1&&y-3>=1)board[x-3][y-3].abandon=0;
            if(x-3>=1&&y+3<=sizey)board[x-3][y+3].abandon=0;
            if(x+3<=sizex&&y-3>=1)board[x+3][y-3].abandon=0;
            if(x+3<=sizex&&y+3<=sizey)board[x+3][y+3].abandon=0;
        }
    }
}

//------------------------------------判断VCF-----------------------------------------//

bool test_VCF(Board board[][MAXSIZE+2],int tryx,int tryy,int thisstep,int &teststeps)
{
    int tempstep=thisstep,nexttryx[n_VCF_MAX],nexttryy[n_VCF_MAX],i,x,y;
    bool find=0,test=0;
    board[tryx][tryy].step=tempstep;
    board[tryx][tryy].whosechess=2-tempstep%2;
    tempstep++;

    //防守方

    for(x=1;x<=sizex;x++)  for(y=1;y<=sizey;y++)            /*qxa1类（己方已有活四或冲四）必胜*/
    {
        if(board[x][y].abandon||board[x][y].whosechess) continue;
        if(value_of_a_qx(board,qxa1,x,y,tempstep))
        {
            clean_temp_change(board,tempstep);
            return 0;
        }
    }

    find=0;
    for(x=1;x<=sizex;x++)   /*qxc2类（对方冲四）走法确定*/
    {
        for(y=1;y<=sizey;y++)
        {
            if(board[x][y].abandon||board[x][y].whosechess) continue;
            if(value_of_a_qx(board,qxc2,x,y,tempstep))
            {
                find=1;break;
            }
        }
        if(find)break;
    }
    if(!find)return 0;
    board[x][y].step=tempstep;
    board[x][y].whosechess=2-tempstep%2;
    tempstep++;

    //进攻方

    for(x=1;x<=sizex;x++)  for(y=1;y<=sizey;y++)            /*qxa1类（己方已有活四或冲四）必胜*/
    {
        if(board[x][y].abandon||board[x][y].whosechess) continue;
        if(value_of_a_qx(board,qxa1,x,y,tempstep))
        {
            clean_temp_change(board,thisstep);
            return 1;
        }
    }

    for(x=1;x<=sizex;x++)  for(y=1;y<=sizey;y++)            /*qxb2类（对方活四）必败*/
    {
        if(board[x][y].abandon||board[x][y].whosechess) continue;
        if(value_of_a_qx(board,qxb2,x,y,tempstep))
        {
            clean_temp_change(board,thisstep);
            return 0;
        }
    }

    for(x=1;x<=sizex;x++)  for(y=1;y<=sizey;y++)            /*qxc2类（对方冲四）走法确定*/
    {
        if(board[x][y].abandon||board[x][y].whosechess) continue;
        if(value_of_a_qx(board,qxc2,x,y,tempstep))
        {
            if(value_of_a_qx(board,qxc1,x,y,tempstep)||value_of_a_qx(board,qxb1,x,y,tempstep))
            {
                teststeps++;
                test=test_VCF(board,x,y,tempstep,teststeps);
                clean_temp_change(board,thisstep);
                return test;
            }
            else
            {
                clean_temp_change(board,thisstep);
                return 0;
            }
        }
    }

    for(x=1;x<=sizex;x++)  for(y=1;y<=sizey;y++)            /*qxb1类（己方活三）必胜*/
    {
        if(board[x][y].abandon||board[x][y].whosechess) continue;
        if(value_of_a_qx(board,qxb1,x,y,tempstep))
        {
            clean_temp_change(board,thisstep);
            return 1;
        }
    }

    for(x=1;x<=sizex;x++)  for(y=1;y<=sizey;y++)                /*四四或四三必胜*/
    {
        if(board[x][y].abandon||board[x][y].whosechess) continue;
        if(checkif_sisi_sisan(board,x,y,tempstep))
        {
            clean_temp_change(board,thisstep);
            return 1;
        }
    }

                                                            /*简单测试是否有VCF（仅测试testnum次）*/
    int testnum=0;
    if(teststeps<=1)        testnum=n_VCF_d2_1;
    else if(teststeps==2)   testnum=n_VCF_d2_2;
    else                    testnum=n_VCF_d2_3;
    if(best_chongsi(board,testnum,nexttryx,nexttryy,tempstep,1))
    {
        teststeps++;
        for(i=0;i<testnum;i++)
        {
            test=test_VCF(board,nexttryx[i],nexttryy[i],tempstep,teststeps);
            if(test)
            {
                clean_temp_change(board,thisstep);
                return 1;
            }
        }
    }
    clean_temp_change(board,thisstep);
    return 0;

}
//------------------------------------棋盘价值计算-----------------------------------------//

float boardvalue(Board board[][MAXSIZE+2],int thisstep,bool skip)
{
    float tempvalue=0.0;
    int i,x,y,tryx[n_VCF_MAX],tryy[n_VCF_MAX],teststeps,num;
//  Board tempboard[MAXSIZE+2][MAXSIZE+2];

    if(!skip)
    for(x=1;x<=sizex;x++)  for(y=1;y<=sizey;y++)            /*qxa1类（己方已有活四或冲四）必胜*/
    {
        if(board[x][y].abandon||board[x][y].whosechess) continue;
        if(value_of_a_qx(board,qxa1,x,y,thisstep))
        {
            return value_win+sizex*sizey-thisstep;
        }
    }

    for(x=1;x<=sizex;x++)  for(y=1;y<=sizey;y++)            /*qxb2类（对方活四）必败*/
    {
        if(board[x][y].abandon||board[x][y].whosechess) continue;
        if(value_of_a_qx(board,qxb2,x,y,thisstep))
        {
            return -(value_win+sizex*sizey-thisstep);
        }
    }

    for(x=1;x<=sizex;x++)  for(y=1;y<=sizey;y++)            /*qxc2类（对方冲四）走法确定*/
    {
        if(board[x][y].abandon||board[x][y].whosechess) continue;
        if(value_of_a_qx(board,qxc2,x,y,thisstep))
        {
            return testpointvalue(board,thisstep,x,y,0);
        }
    }

    if(!skip)
    for(x=1;x<=sizex;x++)  for(y=1;y<=sizey;y++)            /*qxb1类（己方活三）必胜*/
    {
        if(board[x][y].abandon||board[x][y].whosechess) continue;
        if(value_of_a_qx(board,qxb1,x,y,thisstep))
        {
            return value_win+sizex*sizey-thisstep;
        }
    }

    if(!skip)
    for(x=1;x<=sizex;x++)  for(y=1;y<=sizey;y++)                /*四四或四三必胜*/
    {
        if(board[x][y].abandon||board[x][y].whosechess) continue;
        if(checkif_sisi_sisan(board,x,y,thisstep))
        {
            teststeps=1;
            if(test_VCF(board,x,y,thisstep,teststeps))
            {
                return value_win+sizex*sizey-(thisstep+1);
            }
        }
    }

    int nd1,ne1,nf1,ng1,nc2,nd2,ne2,nf2,ng2;
    float value1,value2;
    count_nqx1(board,thisstep,value1,nd1,ne1,nf1,ng1);
    count_nqx2(board,thisstep,value2,nc2,nd2,ne2,nf2,ng2);

/*      if(debuging)
    {
        printboard_num(board);
        printf("试探棋盘数据统计：\n\
己方：value1=%.2f:\tnd1=%d\tne1=%d\tnf1=%d\tng1=%d\n\
对方：value2=%.2f:\tnc2=%d\tnd2=%d\tne2=%d\tnf2=%d\tng2=%d\n",value1,nd1,ne1,nf1,ng1,value2,nc2,nd2,ne2,nf2,ng2);
    } */

    if(ne1>=2&&(!nc2)&&(!nd2))      //（若活二数>=2，且对方无活三或眠三）
    for(x=1;x<=sizex;x++)  for(y=1;y<=sizey;y++)                /*三三必胜*/
    {
        if(board[x][y].abandon||board[x][y].whosechess) continue;
        if(checkif_sansan(board,x,y,thisstep))
        {
            return value_win+sizex*sizey-thisstep;
        }
    }

    if(!skip)                                                                   /*简单测试是否有VCF（仅测试n_VCF_d1_2次）*/
    {
        num=n_VCF_d1_2;
        if(best_chongsi(board,num,tryx,tryy,thisstep,0))
        {
            teststeps=1;
            for(i=0;i<num;i++)
            {
                if(test_VCF(board,tryx[i],tryy[i],thisstep,teststeps))
                {
                    return value_win+sizex*sizey-(thisstep+teststeps);
                }
            }

        }
    }

    int attacking=0; //初步判断优势程度（参见前面【价值表（棋盘已有局面，不计入己方即将落的子）】的说明）
    if(nc2)//1.若对方有活三
    {
        if(nd1)
        {
            if(nc2==1) attacking=-1;    else attacking=-4;
        }
        else
        {
            if(nd2||ne2) attacking=-2;
            else
            {
                if(ne1) attacking=0;    else attacking=-2;
            }
            if(nc2>=2) attacking-=5;
        }
    }
    else if(nd1)//2.若己方有眠三
    {
        if(nd2||ne2)    attacking=1;
        else
        {
            if(ne1) attacking=3;    else attacking=1;
        }
    }
    else if(nd2)//3.若对方有眠三
    {
        if(ne1>=2) attacking=2; else if(ne1==1) attacking=0;else attacking=-2;
        if(nd2==1&&(!ne2)) attacking++;
    }
    else if(ne1)//4.若己方有活二
    {
        if(ne2>ne1) attacking=1;    else attacking=3;
        if(ne1==1)  attacking-=2;
    }
    else if(ne2)//5.若对方有活二
    {
        if(nf1) attacking=-2;   else attacking=-4;
        if(ne2==1)attacking++;
    }
    else if(nf1)//6.若己方有眠二
    {
        if(nf2) attacking=1;    else attacking=2;
    }
    else if(nf2)//7.若对方有眠二
    {
        if(value1>=4) attacking=0;  else attacking=-2;
    }
    else if(value1>=4)//8.若己方有活一
    {
        if(value2>=4)
        {
            if(value1>=value2) attacking=3; else attacking=0;
        }
        else attacking=3;
    }
    else if(value2>=4)//9.若对方有活一
    {
        if(ng2>=8) attacking=-4;
        else if(ng2>=4) attacking=-3;
        else attacking=-2;
    }
    else if(value2>0)//10.若对方有子，但位置不好
    {
        if(ng2>=3) attacking=0;
        else attacking=2;
    }
    else attacking=3;   //双方均无子

    bool sansan_added=0;
    if(ne1>=2&&(!nc2))      //（若活二数>=2，且对方无活三）
    for(x=1;x<=sizex;x++)  for(y=1;y<=sizey;y++)                /*三三阵*/
    {
        if(board[x][y].abandon||board[x][y].whosechess) continue;
        if(checkif_sansan(board,x,y,thisstep))
        {
            if(!sansan_added)
            {
                attacking+=5;sansan_added=1;
            }
            else attacking++;
        }
    }

    tempvalue=value1*pow(attackratio,attacking)-value2/pow(attackratio,attacking);

    return tempvalue;
}

float testpointvalue(Board board[][MAXSIZE+2],int thisstep,int testx,int testy,bool skip)
{
    float value=0.0;
    board[testx][testy].step=thisstep;
    board[testx][testy].whosechess=2-thisstep%2;
    value=-boardvalue(board,thisstep+1,skip);
    board[testx][testy].step=0;
    board[testx][testy].whosechess=0;
    return value;
}

//-------------------------------------------AI---------------------------------------------//
bool noblock()
{
    int x,y;
    for(x=1;x<=sizex;x++) for(y=1;y<=sizey;y++)
    {
        if(realboard[x][y].whosechess)  continue;
        for(y=1;y<=sizey;y++)
        {
            if(realboard[x][y].block)
            {
                return 0;
            }
        }
    }
    return 1;
}

bool AI_depth1(int &move_x,int &move_y,float &thisvalue)
{
    int i,x,y,tryx[n_VCF_MAX],tryy[n_VCF_MAX],teststeps=1,num,count,countequal;
    bool skip,finish_trying=1;
    float tempvalue,tempbestvalue=-111111,easytempbestvalue=-111111;
    node *p_temp;

    if(step==1)//第一步自动下在中间
    {
        if(noblock())
        {
            move_x=(1+sizex+random()*2)/2;
            move_y=(1+sizey+random()*2)/2;
            add_or_find_node(p_real,move_x,move_y,step);
            add_node_information(value_UNCOMPUTED);
            return 1;
        }

    }
    cal_abandon(realboard);

    for(x=1;x<=sizex;x++)  for(y=1;y<=sizey;y++)            /*qxa1类（己方已有活四或冲四）必胜*/
    {
        if(realboard[x][y].abandon||realboard[x][y].whosechess||realboard[x][y].block)  continue;
        if(value_of_a_qx(realboard,qxa1,x,y,step))
        {
            move_x=x;   move_y=y;
            add_or_find_node(p_real,move_x,move_y,step);
            add_node_information(value_win+sizex*sizey-step);
            p_real->all_son=1;
            return 1;
        }
    }

    for(x=1;x<=sizex;x++)  for(y=1;y<=sizey;y++)            /*qxb2类（对方活四）必败*/
    {
        if(realboard[x][y].abandon||realboard[x][y].whosechess||realboard[x][y].block)  continue;
        if(value_of_a_qx(realboard,qxb2,x,y,step))
        {
            move_x=x;   move_y=y;
            add_or_find_node(p_real,move_x,move_y,step);
            add_node_information(-(value_win+sizex*sizey-step));
            p_real->all_son=1;
            return 1;
        }
    }

    for(x=1;x<=sizex;x++)  for(y=1;y<=sizey;y++)            /*qxc2类（对方冲四）走法确定*/
    {
        if(realboard[x][y].abandon||realboard[x][y].whosechess||realboard[x][y].block)  continue;
        if(value_of_a_qx(realboard,qxc2,x,y,step))
        {
            move_x=x;   move_y=y;
            add_or_find_node(p_real,move_x,move_y,step);
            add_node_information(value_UNCOMPUTED);
            p_real->all_son=1;
            return 1;
        }
    }

    for(x=1;x<=sizex;x++)  for(y=1;y<=sizey;y++)            /*qxb1类（己方活三）必胜*/
    {
        if(realboard[x][y].abandon||realboard[x][y].whosechess||realboard[x][y].block)  continue;
        if(value_of_a_qx(realboard,qxb1,x,y,step))
        {
            move_x=x;   move_y=y;
            add_or_find_node(p_real,move_x,move_y,step);
            add_node_information(value_win+sizex*sizey-step);
            p_real->all_son=1;
            return 1;
        }
    }

    for(x=1;x<=sizex;x++)  for(y=1;y<=sizey;y++)                /*四四或四三必胜(需测试对方防守时是否形成四)*/
    {
        if(realboard[x][y].abandon||realboard[x][y].whosechess||realboard[x][y].block)  continue;
        if(checkif_sisi_sisan(realboard,x,y,step))
        {
            teststeps=1;
            if(test_VCF(realboard,x,y,step,teststeps))
            {
                move_x=x;   move_y=y;
                /* if(debuging)printf("VCF win"); */
                add_or_find_node(p_real,move_x,move_y,step);
                add_node_information(value_win+sizex*sizey-(step+teststeps));
                p_real->all_son=1;
                return 1;
            }
        }
    }


    int nd1,ne1,nf1,ng1,nc2,nd2,ne2,nf2,ng2;
    float value1,value2;
    count_nqx1(realboard,step,value1,nd1,ne1,nf1,ng1);
    count_nqx2(realboard,step,value2,nc2,nd2,ne2,nf2,ng2);

/*  if(printmain)
    {
        printf("当前棋盘数据统计：\n\
己方：value1=%.2f:\tnd1=%d\tne1=%d\tnf1=%d\tng1=%d\n\
对方：value2=%.2f:\tnc2=%d\tnd2=%d\tne2=%d\tnf2=%d\tng2=%d\n",value1,nd1,ne1,nf1,ng1,value2,nc2,nd2,ne2,nf2,ng2);
    } */

    if(ne1>=2&&(!nc2)&&(!nd2))      //（若活二数>=2，且对方无活三或眠三）
    for(x=1;x<=sizex;x++)  for(y=1;y<=sizey;y++)                /*三三必胜*/
    {
        if(realboard[x][y].abandon||realboard[x][y].whosechess||realboard[x][y].block)  continue;
        if(checkif_sansan(realboard,x,y,step))
        {
            move_x=x;   move_y=y;
            add_or_find_node(p_real,move_x,move_y,step);
            add_node_information(value_win+sizex*sizey-step);
            p_real->all_son=1;
            return 1;
        }
    }


    num=n_VCF_d1_1;                                         /*简单测试是否有VCF（测试n_VCF_d1_1次）*/
    if(best_chongsi(realboard,num,tryx,tryy,step,1))
    {
        for(i=0;i<num;i++)
        {
            teststeps=1;
            if(test_VCF(realboard,tryx[i],tryy[i],step,teststeps))
            {
                move_x=tryx[i]; move_y=tryy[i];
                /* if(debuging)printf("VCF win"); */
                add_or_find_node(p_real,move_x,move_y,step);
                add_node_information(value_win+sizex*sizey-(step+teststeps));
                p_real->all_son=1;
                return 1;
            }
        }
    }

    //不确定胜负的：以下是深度为1的搜索

    for(x=1;x<=sizex;x++) for(y=1;y<=sizey;y++) realboard[x][y].value=value_ABANDON;    //value数据清空

    if(nc2) //若对方有活三，只能走堵活三或冲四
    {
        for(x=1;x<=sizex;x++)  for(y=1;y<=sizey;y++)            //堵对方活三的走法
        {
            if(realboard[x][y].abandon||realboard[x][y].whosechess||realboard[x][y].block)  continue;
            if(value_of_a_qx(realboard,qxd2,x,y,step))
            {
                realboard[x][y].value=value_UNCOMPUTED;
            }
        }
        if(nd1)//有眠三，允许冲四走法
        for(x=1;x<=sizex;x++)  for(y=1;y<=sizey;y++)
        {
            if(realboard[x][y].abandon||realboard[x][y].whosechess||realboard[x][y].block)  continue;
            if(value_of_a_qx(realboard,qxc1,x,y,step))
            {
                realboard[x][y].value=value_UNCOMPUTED;
            }
        }
    }
    else    //对方无活三
    {
        if(value2>=4)   // 对方已有较好的活子，仅考虑近邻走法
        {
            for(x=1;x<=sizex;x++)  for(y=1;y<=sizey;y++)
            {
                if(realboard[x][y].whosechess||realboard[x][y].block)   continue;
                if(realboard[x][y].abandon==0)  realboard[x][y].value=value_UNCOMPUTED;
            }
        }
        else    // 对方暂无较好的活子，考虑全盘搜索
        {
            for(x=1;x<=sizex;x++)  for(y=1;y<=sizey;y++)
            {
                if(realboard[x][y].whosechess||realboard[x][y].block)   continue;
                realboard[x][y].value=value_UNCOMPUTED;
            }
        }
    }

    if(!nc2)    //让棋，看对方有无简单必胜（若无，skip=1）
    {
        for(x=1;x<=sizex;x++)
        {
            y=1; skip=0;
            if(realboard[x][y].whosechess)  continue;
            tempvalue=testpointvalue(realboard,step,x,y,skip);
            if(abs(tempvalue)<value_win)skip=1;
            break;
        }
    }
    else
    {
        skip=0;
    }

    count=0;tempbestvalue=-111111;
    for(x=1;x<=sizex;x++)
    {
        for(y=1;y<=sizey;y++)
        {
            if(realboard[x][y].whosechess||realboard[x][y].block)   continue;
            if(realboard[x][y].value==value_UNCOMPUTED)
            {
                if(quickanswer||check_timeout()){finish_trying=0;break;}
                if(add_or_find_node(p_real,x,y,step)&&p_new->value!=value_UNCOMPUTED)   //已算过的分支，跳过
                {
                    realboard[x][y].value=p_new->value;
                    continue;
                }
                p_temp=p_new;
                tempvalue=testpointvalue(realboard,step,x,y,skip);
//              printf("%d %d,%f\n",x,y,tempvalue);
                realboard[x][y].value=tempvalue;
                p_new=p_temp;   //恢复p_new,防止在testpointvalue函数中p_new改变
                add_node_information(tempvalue);
                if(tempvalue>-value_win)    count++;
//              printf("p_new->x=%d,p_new->y=%d,p_new->value=%f\n",p_new->x,p_new->y,p_new->value);
                if(tempbestvalue==-111111||tempvalue>tempbestvalue){tempbestvalue=tempvalue;move_x=x;move_y=y;countequal=1;}
                else if(tempvalue==tempbestvalue)
                {
                    countequal++;
                    if(random()<1.0/countequal){tempbestvalue=tempvalue;move_x=x;move_y=y;}
                }
            }
        }
        if(quickanswer){finish_trying=0;break;}
    }
    if(finish_trying)
    {
        cut_branch(p_real,max_keep);    //剪枝
        p_real->all_son=1;
    }
//system("pause");

    easytempbestvalue=-111111;
    if(finish_trying==0)    //没有完成各点尝试，按简单价值取点（时间到）
    {
        countequal=1;
        move_x=-1;
        if(move_x==-1)
        {
            easytempbestvalue=-111111;
            for(x=1;x<=sizex;x++)  for(y=1;y<=sizey;y++)
            {
                if(realboard[x][y].abandon||realboard[x][y].value<=-value_win||realboard[x][y].whosechess||realboard[x][y].block)   continue;
                tempvalue=pointvalue(realboard,x,y,step,2.0*random());  //攻击性随机（略倾向于防守）
                if(easytempbestvalue==-111111||tempvalue>easytempbestvalue){easytempbestvalue=tempvalue;move_x=x;move_y=y;countequal=1;}
                else if(tempvalue==easytempbestvalue)
                {
                    countequal++;
                    if(random()<1.0/countequal){easytempbestvalue=tempvalue;move_x=x;move_y=y;}
                }
            }
        }

        if(move_x==-1)
        {
            easytempbestvalue=-111111;
            for(x=1;x<=sizex;x++)  for(y=1;y<=sizey;y++)
            {
                if(realboard[x][y].abandon||realboard[x][y].whosechess||realboard[x][y].block)  continue;
                tempvalue=pointvalue(realboard,x,y,step,2.0*random());  //攻击性随机（略倾向于防守）
                if(easytempbestvalue==-111111||tempvalue>easytempbestvalue){easytempbestvalue=tempvalue;move_x=x;move_y=y;countequal=1;}
                else if(tempvalue==easytempbestvalue)
                {
                    countequal++;
                    if(random()<1.0/countequal){easytempbestvalue=tempvalue;move_x=x;move_y=y;}
                }
            }
        }
        return 1;
    }


    add_or_find_node(p_real->parent,p_real->x,p_real->y,step-1);
    if(abs(tempbestvalue)!=value_UNCOMPUTED && abs(tempbestvalue)!=value_ABANDON && abs(tempbestvalue)!=111111) p_new->value=-tempbestvalue;
//  printf("-tempbestvalue=%f\n",-tempbestvalue);
//  if(abs(tempbestvalue)>=value_win || count<=1)   return 1;
    return 0;
}

/////////////////////
void AI_trypoint_depth1(Board tempboard[][MAXSIZE+2],int tempstep)
{
    int i,x,y,move_x,move_y,tryx[n_VCF_MAX],tryy[n_VCF_MAX],teststeps=1,num,count,countequal;
    bool skip,finish_trying=1;
    float tempvalue,tempbestvalue=-111111,easytempbestvalue=-111111;
    node *p_temp;

    cal_abandon(tempboard);

    for(x=1;x<=sizex;x++)  for(y=1;y<=sizey;y++)            /*qxa1类（己方已有活四或冲四）必胜*/
    {
        if(tempboard[x][y].abandon||tempboard[x][y].whosechess) continue;
        if(value_of_a_qx(tempboard,qxa1,x,y,tempstep))
        {
            move_x=x;   move_y=y;
            add_or_find_node(p_now,move_x,move_y,tempstep);
            add_node_information(value_win+sizex*sizey-tempstep);
            p_now->all_son=1;
            return;
        }
    }

    for(x=1;x<=sizex;x++)  for(y=1;y<=sizey;y++)            /*qxb2类（对方活四）必败*/
    {
        if(tempboard[x][y].abandon||tempboard[x][y].whosechess) continue;
        if(value_of_a_qx(tempboard,qxb2,x,y,tempstep))
        {
            move_x=x;   move_y=y;
            add_or_find_node(p_now,move_x,move_y,tempstep);
            add_node_information(-(value_win+sizex*sizey-tempstep));
            p_now->all_son=1;
            return;
        }
    }

    for(x=1;x<=sizex;x++)  for(y=1;y<=sizey;y++)            /*qxc2类（对方冲四）走法确定*/
    {
        if(tempboard[x][y].abandon||tempboard[x][y].whosechess) continue;
        if(value_of_a_qx(tempboard,qxc2,x,y,tempstep))
        {
            move_x=x;   move_y=y;
            add_or_find_node(p_now,move_x,move_y,tempstep);
            add_node_information(value_UNCOMPUTED);
            p_now->all_son=1;
            return;
        }
    }

    for(x=1;x<=sizex;x++)  for(y=1;y<=sizey;y++)            /*qxb1类（己方活三）必胜*/
    {
        if(tempboard[x][y].abandon||tempboard[x][y].whosechess) continue;
        if(value_of_a_qx(tempboard,qxb1,x,y,tempstep))
        {
            move_x=x;   move_y=y;
            add_or_find_node(p_now,move_x,move_y,tempstep);
            add_node_information(value_win+sizex*sizey-tempstep);
            p_now->all_son=1;
            return;
        }
    }

    for(x=1;x<=sizex;x++)  for(y=1;y<=sizey;y++)                /*四四或四三必胜(需测试对方防守时是否形成四)*/
    {
        if(check_timeout())return;
        if(tempboard[x][y].abandon||tempboard[x][y].whosechess) continue;
        if(checkif_sisi_sisan(tempboard,x,y,tempstep))
        {
            teststeps=1;
            if(test_VCF(tempboard,x,y,tempstep,teststeps))
            {
                move_x=x;   move_y=y;
                /* if(debuging)printf("VCF win"); */
                add_or_find_node(p_now,move_x,move_y,tempstep);
                add_node_information(value_win+sizex*sizey-(tempstep+teststeps));
                p_now->all_son=1;
                return;
            }
        }
    }


    if(check_timeout())return;

    int nd1,ne1,nf1,ng1,nc2,nd2,ne2,nf2,ng2;
    float value1,value2;
    count_nqx1(tempboard,tempstep,value1,nd1,ne1,nf1,ng1);
    count_nqx2(tempboard,tempstep,value2,nc2,nd2,ne2,nf2,ng2);

/*  if(printmain)
    {
        printf("当前棋盘数据统计：\n\
己方：value1=%.2f:\tnd1=%d\tne1=%d\tnf1=%d\tng1=%d\n\
对方：value2=%.2f:\tnc2=%d\tnd2=%d\tne2=%d\tnf2=%d\tng2=%d\n",value1,nd1,ne1,nf1,ng1,value2,nc2,nd2,ne2,nf2,ng2);
    }*/

    if(ne1>=2&&(!nc2)&&(!nd2))      //（若活二数>=2，且对方无活三或眠三）
    for(x=1;x<=sizex;x++)  for(y=1;y<=sizey;y++)                /*三三必胜*/
    {
        if(check_timeout())return;
        if(tempboard[x][y].abandon||tempboard[x][y].whosechess) continue;
        if(checkif_sansan(tempboard,x,y,tempstep))
        {
            move_x=x;   move_y=y;
            add_or_find_node(p_now,move_x,move_y,tempstep);
            add_node_information(value_win+sizex*sizey-tempstep);
            p_now->all_son=1;
            return;
        }
    }


    num=n_VCF_d1_2;                                         /*简单测试是否有VCF（测试n_VCF_d1_2次）*/
    if(best_chongsi(tempboard,num,tryx,tryy,tempstep,0))
    {
        for(i=0;i<num;i++)
        {
            if(check_timeout())return;
            teststeps=1;
            if(test_VCF(tempboard,tryx[i],tryy[i],tempstep,teststeps))
            {
                move_x=tryx[i]; move_y=tryy[i];
            /* //   if(debuging)printf("VCF win"); */
                add_or_find_node(p_now,move_x,move_y,tempstep);
                add_node_information(value_win+sizex*sizey-(tempstep+teststeps));
                p_now->all_son=1;
                return;
            }
        }
    }

    if(check_timeout())return;
    //不确定胜负的：以下是深度为1的搜索

    for(x=1;x<=sizex;x++) for(y=1;y<=sizey;y++) tempboard[x][y].value=value_ABANDON;    //value数据清空

    if(nc2) //若对方有活三，只能走堵活三或冲四
    {
        for(x=1;x<=sizex;x++)  for(y=1;y<=sizey;y++)            //堵对方活三的走法
        {
            if(tempboard[x][y].abandon||tempboard[x][y].whosechess) continue;
            if(value_of_a_qx(tempboard,qxd2,x,y,tempstep))
            {
                tempboard[x][y].value=value_UNCOMPUTED;
            }
        }
        if(nd1)//有眠三，允许冲四走法
        for(x=1;x<=sizex;x++)  for(y=1;y<=sizey;y++)
        {
            if(tempboard[x][y].abandon||tempboard[x][y].whosechess) continue;
            if(value_of_a_qx(tempboard,qxc1,x,y,tempstep))
            {
                tempboard[x][y].value=value_UNCOMPUTED;
            }
        }
    }
    else    //对方无活三
    {
        if(value2>=4)   // 对方已有较好的活子，仅考虑近邻走法
        {
            for(x=1;x<=sizex;x++)  for(y=1;y<=sizey;y++)
            {
                if(tempboard[x][y].whosechess)  continue;
                if(tempboard[x][y].abandon==0)  tempboard[x][y].value=value_UNCOMPUTED;
            }
        }
        else    // 对方暂无较好的活子，考虑全盘搜索
        {
            for(x=1;x<=sizex;x++)  for(y=1;y<=sizey;y++)
            {
                if(tempboard[x][y].whosechess)  continue;
                tempboard[x][y].value=value_UNCOMPUTED;
            }
        }
    }

    if(!nc2)    //让棋，看对方有无简单必胜（若无，skip=1）
    {
        for(x=1;x<=sizex;x++)
        {
            y=1; skip=0;
            if(tempboard[x][y].whosechess)  continue;
            tempvalue=testpointvalue(tempboard,tempstep,x,y,skip);
            if(abs(tempvalue)<value_win)skip=1;
            break;
        }
    }
    else
    {
        skip=0;
    }

    count=0;tempbestvalue=-111111;
    for(x=1;x<=sizex;x++)
    {
        for(y=1;y<=sizey;y++)
        {
            if(tempboard[x][y].whosechess)  continue;
            if(tempboard[x][y].value==value_UNCOMPUTED)
            {
                if(quickanswer||check_timeout()){finish_trying=0;return;}
                if(add_or_find_node(p_now,x,y,tempstep)&&p_new->value!=value_UNCOMPUTED)    //已算过的分支，跳过
                {
                    tempboard[x][y].value=p_new->value;
                    continue;
                }
                p_temp=p_new;
                tempvalue=testpointvalue(tempboard,tempstep,x,y,skip);
                tempboard[x][y].value=tempvalue;
                p_new=p_temp;   //恢复p_new,防止在testpointvalue函数中p_new改变
                add_node_information(tempvalue);
                if(tempvalue>-value_win)    count++;
//              printf("p_new->x=%d,p_new->y=%d,p_new->value=%f\n",p_new->x,p_new->y,p_new->value);
                if(tempbestvalue==-111111||tempvalue>tempbestvalue){tempbestvalue=tempvalue;move_x=x;move_y=y;countequal=1;}
                else if(tempvalue==tempbestvalue)
                {
                    countequal++;
                    if(random()<1.0/countequal){tempbestvalue=tempvalue;move_x=x;move_y=y;}
                }
            }
        }
        if(quickanswer){finish_trying=0;return;}
    }
    if(finish_trying)
    {
        p_now->all_son=1;
        cut_branch(p_now,max_keep); //剪枝
    }
//system("pause");

//  if(finish_trying==0)    //没有完成各点尝试，按简单价值取点（时间到）
/// {
//      return;
//  }


//  add_or_find_node(p_now->parent,p_now->x,p_now->y,tempstep-1);
//  p_new->value=-tempbestvalue;
//  printf("-tempbestvalue=%f\n",-tempbestvalue);
//  if(abs(tempbestvalue)>=value_win || count<=1)   return;
//  return;
}

bool check_certain(int &move_x,int &move_y) //更新棋盘value信息，并检查是否已确定走法
{
    int count,countequal,x,y;
    float tempbestvalue,tempvalue,easytempbestvalue=-111111;
    bool have_uncomputed=0;
    node *ptemp;

    if(!p_real->all_son){printf("ERROR1 in check_certain: not all_son!\n");system("pause");return 0;}
    if(p_real->next==NULL || p_real->next->nodepiecenum<=p_real->nodepiecenum) {printf("ERROR2 in check_certain: no son!\n");system("pause");return 0;}

    for(x=1;x<=sizex;x++) for(y=1;y<=sizey;y++) realboard[x][y].value=value_ABANDON;    //value数据清空

    count=0;    countequal=0;   tempbestvalue=-111111;
    for(ptemp=p_real->next;ptemp!=NULL&&ptemp->nodepiecenum==p_real->nodepiecenum+1;ptemp=ptemp->right_brother)
    {
        tempvalue=ptemp->value;
        realboard[ptemp->x][ptemp->y].value=tempvalue;
        if(ptemp->value==value_UNCOMPUTED){ have_uncomputed=1;tempvalue=-value_win+1;}
        if(ptemp->value>-value_win) count++;
        if(tempbestvalue==-111111||tempvalue>tempbestvalue){tempbestvalue=tempvalue;move_x=ptemp->x;move_y=ptemp->y;countequal=1;}
    //  else if(tempvalue==tempbestvalue)
    //  {
    //      countequal++;
    //      if(random()<1.0/countequal){tempbestvalue=tempvalue;move_x=ptemp->x;move_y=ptemp->y;}
    //  }
    }
    if(!have_uncomputed||(p_real->value!=value_UNCOMPUTED&&-tempbestvalue!=-value_win+1&&-tempbestvalue>p_real->value))
    {
        add_or_find_node(p_real->parent,p_real->x,p_real->y,step-1);
        p_new->value=-tempbestvalue;
    }
    if(abs(tempbestvalue)>=value_win && countequal>=2)  //必败（或必胜）且步数相同的，取简单价值最优
    {
        countequal=1;
        for(x=1;x<=sizex;x++)  for(y=1;y<=sizey;y++)
        {
            if(realboard[x][y].abandon||realboard[x][y].value!=tempbestvalue||realboard[x][y].whosechess||realboard[x][y].block)    continue;
            tempvalue=pointvalue(realboard,x,y,step,2.0*random());  //攻击性随机（略倾向于防守）
            if(easytempbestvalue==-111111||tempvalue>easytempbestvalue){easytempbestvalue=tempvalue;move_x=x;move_y=y;countequal=1;}
            else if(tempvalue==easytempbestvalue)
            {
                countequal++;
                if(random()<1.0/countequal){easytempbestvalue=tempvalue;move_x=x;move_y=y;}
            }
        }
        return 1;
    }
    if(count<=1||tempbestvalue>=value_win)  //至多有一个非必败点，或已有必胜，可以立即返回结果
    {
        return 1;
    }
    count=0;
    for(ptemp=p_real->next;ptemp!=NULL&&ptemp->nodepiecenum==p_real->nodepiecenum+1;ptemp=ptemp->right_brother)
    {
        if(ptemp->value>=tempbestvalue-max_gap_return)  count++;
    }
    if(count<=1)    //差距悬殊，可以立即返回结果
    {
        return 1;
    }
    return 0;
}


bool find_whichtocompute(bool whetherbalance,int &bestx,int &besty)
{
    if(!p_now->all_son){bestx=-1;besty=-1;return 0;}
    if(p_now->next==NULL||p_now->next->nodepiecenum<p_now->nodepiecenum){printf("ERROR1 in find_whichtocompute\n"); system("pause"); return 0;}

    node *ptemp;
    int nolose=0,nolose_other=0;
    float tempbestvalue=-111111,tempvalue;

    for(ptemp=p_now->next;ptemp!=NULL&&ptemp->nodepiecenum==p_now->nodepiecenum+1;ptemp=ptemp->right_brother)
    {
        tempvalue=ptemp->value;
        if(ptemp->value==value_UNCOMPUTED){bestx=ptemp->x;besty=ptemp->y;return 1;}
        nolose=(ptemp->nodepiecenum%2)?(ptemp->noloseblack):(ptemp->nolosewhite);
        nolose_other=(ptemp->nodepiecenum%2)?(ptemp->nolosewhite):(ptemp->noloseblack);
        tempvalue=tempvalue-value_depth*(log((float)max((float)nolose_other,(float)1.0))+log((float)max(nolose/(float)ratio_bw,(float)1.0)))/log(2.0);
        if(tempbestvalue==-111111||tempvalue>tempbestvalue){tempbestvalue=tempvalue;bestx=ptemp->x;besty=ptemp->y;}
    }
    return 1;

}
void makemove_temp(Board board[][MAXSIZE+2],int x,int y,int &totalstep)
{
    bool is_saved=0;
    is_saved=add_or_find_node(p_now,x,y,totalstep);
    p_now=p_new;
    if(board[x][y].whosechess)  {printf("ERROR in makemove_temp\n");system("pause");}
    else    {board[x][y].step=totalstep;    board[x][y].whosechess=2-(totalstep%2); totalstep=totalstep+1;}
}
void de_move_temp(Board board[][MAXSIZE+2],int &totalstep)
{
    int x,y;
    p_now=p_now->parent;
    totalstep--;
    for(x=1;x<=sizex;x++) for(y=1;y<=sizey;y++)
    {
        if(board[x][y].step>=totalstep){board[x][y].step=0;board[x][y].whosechess=0;}
    }
}

void back_to_start(bool whetherbalance,bool onlybacktochange,Board tempboard[][MAXSIZE+2],int &tempstep)//逐步修正value值和depth值;onlybacktochange表示退到棋盘信息不变的位置就不再退    ***
{
    float tempbestvalue=-111111,tempvalue,fvalue;
    node *ptemp;
    long int noloseblack=0,nolosewhite=0,tempbestnolose,tempnolose;
    int count;

    while(p_now->all_son)
    {
/* if(debuging_wherebug)        printf("bts_%d\n",p_now->nodepiecenum); */
        noloseblack=0;  nolosewhite=0;  tempbestvalue=-111111;  count=0;
        for(ptemp=p_now->next;ptemp!=NULL&&ptemp->nodepiecenum==p_now->nodepiecenum+1;ptemp=ptemp->right_brother)
        {
            tempvalue=ptemp->value;
            count++;
            if(ptemp->value==value_UNCOMPUTED)return;
            if(tempbestvalue==-111111||tempvalue>tempbestvalue){tempbestvalue=tempvalue;}
        }
    //  if(count>max_keep)cut_branch(p_now,max_keep);
/* if(debuging_wherebug)printf("a"); */
        if(tempbestvalue>=value_win)
        {
/* if(debuging_wherebug)printf("b"); */
            if(p_now->next->nodepiecenum%2) nolosewhite=0;  else noloseblack=0;
            for(ptemp=p_now->next;ptemp!=NULL&&ptemp->nodepiecenum==p_now->nodepiecenum+1;ptemp=ptemp->right_brother)
            {
                if(ptemp->value>=0){if(ptemp->nodepiecenum%2) noloseblack+=ptemp->noloseblack;  else nolosewhite+=ptemp->nolosewhite;}
            }
        }
        else if(tempbestvalue<=-value_win)
        {
/* if(debuging_wherebug)printf("c"); */
            if(p_now->next->nodepiecenum%2) noloseblack=0;  else nolosewhite=0;
            if(p_now->next->nodepiecenum%2) nolosewhite=268435455;  else nolosewhite=268435455; //允许的最大值
/* if(debuging_wherebug)printf("C"); */
            for(ptemp=p_now->next;ptemp!=NULL&&ptemp->nodepiecenum==p_now->nodepiecenum+1;ptemp=ptemp->right_brother)
            {
                if(ptemp->value==tempbestvalue){if(ptemp->nodepiecenum%2) nolosewhite=min(nolosewhite,ptemp->nolosewhite);  else noloseblack=min(noloseblack,ptemp->noloseblack);}
            }
/* if(debuging_wherebug)printf("c"); */
        }
        else
        {
/* if(debuging_wherebug)printf("d"); */
            tempbestnolose=268435455;   //允许的最大值
            for(ptemp=p_now->next;ptemp!=NULL&&ptemp->nodepiecenum==p_now->nodepiecenum+1;ptemp=ptemp->right_brother)
            {
                fvalue=ptemp->value;
                if(abs(fvalue)>value_win)continue;
                if(fvalue<-120&&tempbestvalue>=-120)continue;
                if(tempbestvalue-fvalue>0&&tempbestvalue<-120)continue;
                if(tempbestvalue-fvalue>20&&tempbestvalue<-80)continue;
        tempnolose=(max((float)(ptemp->nodepiecenum%2)?(float)(ptemp->nolosewhite):(float)(ptemp->noloseblack),(float)1.0))*pow((float)2.0,(tempbestvalue-fvalue)/(float)value_depth);
                if(tempnolose<1)tempnolose=1;
                if(tempnolose<tempbestnolose)
                {
                        tempbestnolose=tempnolose;
                }
            }
/* if(debuging_wherebug)printf("e"); */
            if(tempbestnolose<1)    tempbestnolose=1;
            if(p_now->next->nodepiecenum%2) nolosewhite=tempbestnolose; else noloseblack=tempbestnolose;


            for(ptemp=p_now->next;ptemp!=NULL&&ptemp->nodepiecenum==p_now->nodepiecenum+1;ptemp=ptemp->right_brother)
            {
                fvalue=ptemp->value;
                if(fvalue<-value_win)continue;
                if(fvalue>=0){if(ptemp->nodepiecenum%2) noloseblack+=ptemp->noloseblack;    else nolosewhite+=ptemp->nolosewhite; continue;}
                if(tempbestvalue-fvalue>100)continue;
                if(fvalue<-100&&tempbestvalue-fvalue>30)continue;
                if(ptemp->nodepiecenum%2) noloseblack+=ptemp->noloseblack;  else nolosewhite+=ptemp->nolosewhite;
            }
/* if(debuging_wherebug)printf("f"); */
        }


/* if(debuging_wherebug)printf("g"); */
        if(onlybacktochange && abs(p_now->value+tempbestvalue)<0.1 && p_now->noloseblack==noloseblack && p_now->nolosewhite==nolosewhite) return;   //该局面信息无变化

/* if(debuging_wherebug)printf("h"); */
        p_now->value=-tempbestvalue;    p_now->noloseblack=noloseblack;     p_now->nolosewhite=nolosewhite;

/* if(debuging_wherebug)printf("i"); */
        if(p_now->nodepiecenum > p_real->nodepiecenum) de_move_temp(tempboard,tempstep);
        else return;

    }

}

void quickreturn(int &move_x,int &move_y)
{
    int x,y,countequal;
    float easytempbestvalue,tempvalue;
    easytempbestvalue=-111111;
    for(x=1;x<=sizex;x++)  for(y=1;y<=sizey;y++)
    {
        if(realboard[x][y].abandon||realboard[x][y].whosechess||realboard[x][y].block)  continue;
        tempvalue=pointvalue(realboard,x,y,step,2.0*random());  //攻击性随机（略倾向于防守）
        if(easytempbestvalue==-111111||tempvalue>easytempbestvalue){easytempbestvalue=tempvalue;move_x=x;move_y=y;countequal=1;}
        else if(tempvalue==easytempbestvalue)
        {
            countequal++;
            if(random()<1.0/countequal){easytempbestvalue=tempvalue;move_x=x;move_y=y;}
        }
    }
}

void AI(int &move_x,int &move_y)
{
    bool iscertain=0,firstcompute=1,havechange=1,board_symmetry[8],whetherbalance=0;
    float value=value_UNCOMPUTED,tempbestvalue=-111111;
    int start_node,i=0,x,y,try_n;
    int tempstep=step;
    Board tempboard[MAXSIZE+2][MAXSIZE+2];

    oldtime=GetTickCount(); //计时开始
    if(engine)
    {
        n_AI++;
        oldtime=engine_start_time;  timeout_left=engine_timeout_left;
/*      printmain=0;
        printmain2=0;
        printmain3=0; */
/*      debuging=0;
        debuging2=0;
        debuging3=0;
        debuging_wherebug=0;
        debuging_draw_new_thinking=0; */
        if(n_AI==1)oldtime-=firstAI_time_cost;
    }
    start_node=total_node;
    quickanswer=0;

    if(sym_random) get_board_symmetry(realboard,board_symmetry);    //判断当前棋盘对称性（若有对称性，输出结果可以在几个对称的走法中随机取）

    if(!p_real->all_son)    //该局面尚未完成深度为1的初步计算
    {
        iscertain=AI_depth1(move_x,move_y,value);
        if(iscertain||quickanswer)
        {
            if(sym_random) random_symmetry(realboard,board_symmetry,move_x,move_y);
            /* if(printmain)    printboard_value(realboard); */
            if(!(move_x>=1&&move_x<=sizex&&move_y>=1&&move_y<=sizey)||realboard[move_x][move_y].whosechess||realboard[move_x][move_y].block) quickreturn(move_x,move_y);    //返回值有误，重新算
            count_timeout_left(GetTickCount()-oldtime);
            return;
        }
    }

    p_now=p_real;
    for(x=0;x<=sizex+1;x++) for(y=0;y<=sizey+1;y++) {tempboard[x][y].whosechess=realboard[x][y].whosechess;tempboard[x][y].step=realboard[x][y].step;}

    try_n=0;
    while(!quickanswer)
    {
    //  tempbestx=-1;
/* if(debuging_wherebug)write_all_node(); */
//if(debuging_wherebug)system("pause");
        if(p_now->nodepiecenum==p_real->nodepiecenum)havechange=1;
/* if(debuging_wherebug)printf("1\n"); */
        if(firstcompute)
        {
            if(check_certain(move_x,move_y))
            {
/* if(debuging_wherebug)printf("2_1\n");     */
                if(sym_random) random_symmetry(realboard,board_symmetry,move_x,move_y);
/* if(debuging_wherebug)printf("3_1\n");     */
                /* if(printmain)    printboard_value(realboard); */
/* if(debuging_wherebug)printf("4_1\n");     */
                if(!(move_x>=1&&move_x<=sizex&&move_y>=1&&move_y<=sizey)||realboard[move_x][move_y].whosechess||realboard[move_x][move_y].block) quickreturn(move_x,move_y);    //返回值有误，重新算
                count_timeout_left(GetTickCount()-oldtime);
/* if(debuging_wherebug)printf("5_1\n");     */
                return;
            }
            /* else if(printmain2)  printboard_value(realboard); */
        }
        else if(havechange)
        {
            if(check_certain(move_x,move_y))
            {
/* if(debuging_wherebug)printf("2_2\n");     */
            //  move_x=tempbestx;move_y=tempbesty;
                if(sym_random) random_symmetry(realboard,board_symmetry,move_x,move_y);
/* if(debuging_wherebug)printf("3_2\n");     */
                /* if(printmain)    printboard_value(realboard); */
/* if(debuging_wherebug)printf("4_2\n");     */
                if(!(move_x>=1&&move_x<=sizex&&move_y>=1&&move_y<=sizey)||realboard[move_x][move_y].whosechess||realboard[move_x][move_y].block) quickreturn(move_x,move_y);    //返回值有误，重新算
                count_timeout_left(GetTickCount()-oldtime);
/* if(debuging_wherebug)printf("5_2\n");     */
                return;
            }
            /* else if(printmain3)  printboard_value(realboard); */
        }
/* if(debuging_wherebug)printf("6\n");   */
        firstcompute=0; havechange=0;
/* if(debuging_wherebug)printf("7\n");   */

        find_whichtocompute(0,x,y);
//      if(tempbestx>=1 && p_now->nodepiecenum==p_real->nodepiecenum && (x!=tempbestx||y!=tempbesty)){move_x=tempbestx;move_y=tempbesty;}//这时最优点比较可靠，暂时保存
/* if(debuging_wherebug)printf("8\n");   */

        if(x<0)//计算当前局面（depth=1）
        {
/* if(debuging_wherebug)printf("9\n");   */
            AI_trypoint_depth1(tempboard,tempstep);
            if(quickanswer)
            {
                if(sym_random) random_symmetry(realboard,board_symmetry,move_x,move_y);
                /* if(printmain)    printboard_value(realboard); */
                if(!(move_x>=1&&move_x<=sizex&&move_y>=1&&move_y<=sizey)||realboard[move_x][move_y].whosechess||realboard[move_x][move_y].block) quickreturn(move_x,move_y);    //返回值有误，重新算
                count_timeout_left(GetTickCount()-oldtime);
                return;
            }
//      if(debuging_draw_new_thinking)write_all_node();
/*      if(debuging_draw_new_thinking)printboard_num(tempboard);
        if(debuging_draw_new_thinking)printboard_value(tempboard); */
//      if(debuging_draw_new_thinking)system("pause");
/* if(debuging_wherebug)    printf("10\n");  */
            if(try_n%10==0) back_to_start(whetherbalance,0,tempboard,tempstep);//一步步返回思考开始的局面，每一步修正棋盘结论。
            else back_to_start(whetherbalance,1,tempboard,tempstep);//返回至棋盘记录发生变化的局面，每一步修正棋盘结论。
/* if(debuging_wherebug)printf("11\n");  */
            try_n++;
        }
        else
        {
/* if(debuging_wherebug)printf("12\n");  */
            makemove_temp(tempboard,x,y,tempstep); //试探走一步
/* if(debuging_wherebug)printf("13\n");  */
        }
/* if(debuging_wherebug)printf("14\n");  */
        if(check_timeout())break;
/* if(debuging_wherebug)printf("15\n");  */
        if(total_node-start_node>=maxn_node_each)break;
/* if(debuging_wherebug)printf("16\n");  */
    }


/* if(debuging_wherebug)printf("17\n");  */
//  check_certain(tempbestx,tempbesty);
    if(sym_random) random_symmetry(realboard,board_symmetry,move_x,move_y);
/* if(debuging_wherebug)printf("18\n");  */
    /* if(printmain)    printboard_value(realboard); */
    if(!(move_x>=1&&move_x<=sizex&&move_y>=1&&move_y<=sizey)||realboard[move_x][move_y].whosechess||realboard[move_x][move_y].block) quickreturn(move_x,move_y);    //返回值有误，重新算
    count_timeout_left(GetTickCount()-oldtime);
    /* if(printmain2)   printf("try_n=%d,total_node=%d,timeleft=%d,value=%f\n",try_n,total_node,timeout_left,-p_real->value); */
    return;
}

void nextchess()
{
    int x=0,y=0,thisstep;
    bool find=0;
    char c1[2]={'z','z'},c2[2]={'z','z'};
    while(1)
    {
        c1[0]='z';c1[1]='z';c2[0]='z';c2[1]='z';
        fflush(stdin);
        while((c1[0]=getchar())=='\n')fflush(stdin);
        if(c1[0]=='s'||c1[0]=='S')
        {
            printboard_step(realboard);c1[0]='z';c2[0]='z';
            continue;
        }
        if(c1[0]=='u'||c1[0]=='U')
        {
            thisstep=step-1;
            find=0;
            for(x=1;x<=sizex;x++)
            {
                for(y=1;y<=sizey;y++)
                {
                    if(realboard[x][y].step==thisstep)  {find=1;    break;}
                }
                if(find)break;
            }
            de_move(x,y,thisstep);
            break;
        }
        if(c1[0]=='d'||c1[0]=='D')
        {
            /* if(debuging) debuging=0;else debuging=1;  */
            continue;
        }
        if(c1[0]=='r'||c1[0]=='R')
        {
            printf("确定重新开始? 输入Y重新开始\n");
            fflush(stdin);
            while((c1[0]=getchar())=='\n')fflush(stdin);
            if(c1[0]=='y'||c1[0]=='Y'){ restart(sizex,sizey);break;}
            continue;
        }
        if((c1[0]>='a'&&c1[0]<='z')||(c1[0]>='A'&&c1[0]<='Z')||c1[0]==' ')
        {
            AI(x,y);
            thisstep=step;
            if(!realboard[x][y].whosechess){ makemove(x,y,thisstep);}
            else{printf("ERROR move:x=%d,y=%d\n",x,y);system("pause");}
            c1[0]='z';c2[0]='z';
            break;
        }
        c1[1]=getchar();
        if((c1[1]>='a'&&c1[1]<='z')||(c1[1]>='A'&&c1[1]<='Z'))
        {
            AI(x,y);
            thisstep=step;
            if(!realboard[x][y].whosechess){makemove(x,y,thisstep);}
            else{printf("ERROR move:x=%d,y=%d\n",x,y);system("pause");}
            break;
        }
        if(!(c1[1]>='0'&&c1[1]<='9')) c1[1]='z';
        while(!(c2[0]>='0'&&c2[0]<='9'))    c2[0]=getchar();
        c2[1]=getchar();
        if(!(c2[1]>='0'&&c2[1]<='9')) c2[1]='z';
        if((c1[0]>='0'&&c1[0]<='9')&&c1[1]=='z')    x=int(c1[0]-'0');
        else if((c1[0]>='0'&&c1[0]<='9')&&(c1[1]>='0'&&c1[1]<='9')) x=10*(int(c1[0]-'0'))+int(c1[1]-'0');
        else {printf("数据无效，请重新输入。\n");continue;}
        if((c2[0]>='0'&&c2[0]<='9')&&c2[1]=='z')    y=int(c2[0]-'0');
        else if((c2[0]>='0'&&c2[0]<='9')&&(c2[1]>='0'&&c2[1]<='9')) y=10*(int(c2[0]-'0'))+int(c2[1]-'0');
        else {printf("数据无效，请重新输入。\n");continue;}
        if(!(x>=1&&x<=sizex&&y>=1&&y<=sizey)){printf("该点不在棋盘内，请重新输入。\n");continue;}
        if(realboard[x][y].whosechess){printf("该点已有棋子，请重新输入。\n");continue;}
        else{thisstep=step; makemove(x,y,thisstep); break;}
    }fflush(stdin);
}

//----------------------------------用于交互信息--------------------------------------//
void makemove(int x,int y,int totalstep)
{
    bool is_saved=0;
    if(!realboard[x][y].whosechess)
    {realboard[x][y].step=totalstep;    realboard[x][y].whosechess=2-(totalstep%2); step=totalstep+1;}

    is_saved=add_or_find_node(p_real,x,y,totalstep);

/*  if(debuging2)
    {
        //write_all_node();
        if(p_new==NULL)printf("p_new==NULL\n");
        else    printf("nodestep::%d,%d\n",p_real->nodepiecenum,p_new->nodepiecenum);
        printf("total_node=%d\n",total_node);
        if(is_saved)printf("找到存储棋形\n"); else printf("未找到存储棋形\n");
    } */

//write_all_node();
    p_real=p_new;
/*  FILE *out;
    if((out = fopen("S:\\log.txt", "a")) != NULL)
    {
        int i,j;
        fprintf(out,"  ");
        for(j=1;j<=sizey;j++)   fprintf(out,"%2d",j);fprintf(out,"\n");
        for(i=1;i<=sizex;i++)
        {
            fprintf(out,"%2d",i);
            for(j=1;j<=MAXSIZE;j++)
            {if(realboard[j][i].whosechess==1)  fprintf(out,"○");
            else if(realboard[j][i].whosechess==2)  fprintf(out,"●");
            else if(realboard[j][i].whosechess==0)  fprintf(out,"┼");
            else fprintf(out,"%2d",realboard[j][i].whosechess);
            }
            fprintf(out,"%2d",i);
            fprintf(out,"\n");
        }
        fprintf(out,"  ");
        for(j=1;j<=MAXSIZE;j++) fprintf(out,"%2d",j);fprintf(out,"\n");
        fclose(out);
    }
    */
}
void de_move(int x,int y,int totalstep)
{
    realboard[x][y].step=0;realboard[x][y].whosechess=0;
    step--;
    if(step!=totalstep){printf("ERROR in de_move:step=%d,totalstep=%d\n",step,totalstep);}
    p_real=p_real->parent;
/*  if(debuging2)
    {
        if(p_new==NULL)printf("p_new==NULL\n");
        else    printf("nodestep::%d,%d\n",p_real->nodepiecenum,p_new->nodepiecenum);
        printf("total_node=%d\n",total_node);
    } */
}

void set_timeout_match(int info_timeout_match)
{
    timeout_match=info_timeout_match;
}

void set_timeout_turn(int info_timeout_turn)
{
    timeout_turn=info_timeout_turn;
}

void set_timeout_left(int info_timeout_left)
{
    timeout_left=info_timeout_left;
}

void set_engine_start_time(int start_time)
{
    engine_start_time=start_time;
}

void set_engine_timeout_left(int timeout_left)
{
    engine_timeout_left=timeout_left;
}

void count_timeout_left(int timeelapsed)
{
    timeout_left-=timeelapsed;
}


void quit_game()
{
    forget_all_node();
}
//////////////////////////////////////////////////////////////////////////////////////////
void main0()
{
    int lastx,lasty,thisstep,x,y;
    bool find;
    restart(sizex,sizey);
    printf("每次落子时，请输入两个数字（1-%d 1-%d）,依次为横纵坐标（左小右大，上小下大）。\n\
两个数字用空格分开，输入结束后按回车。\n\
R:重新开始。U：悔棋。C：电脑换一种下法。\n\
S:输出每点对应的步数 D:在是否debug状态间切换\n\
输入空格或其他字母，则电脑思考下一步。\n\
1为黑棋，2为白棋，0为空位。\n",sizex,sizey);/*R:重新开始。U：悔棋。C：电脑换一种下法。P:电脑停止运算，输出目前最佳选择。\n\*/
    while(1)
    {
        if(step%2)  {printf("第%2d步 黑棋 ",step);nextchess();}
        else        {printf("第%2d步 白棋 ",step);nextchess();}
        printboard_num(realboard);
        //printboard_step(realboard);
        //printf("step=%d\n",step);
        if(win(realboard,lastx=getx(realboard,step-1),lasty=gety(realboard,step-1),2-(step-1)%2)||step-1>=NUMMOST)
        {
            if(step-1>=NUMMOST) {printf("和棋\n");break;}
            else if((step-1)%2)printf("黑棋胜\n");else printf("白棋胜\n");
            char c;fflush(stdin);
            while((c=getchar())=='\n')fflush(stdin);
            printf("按R键重新开始，按E键退出，按U键悔棋\n");
            while(1)
            {
                if(kbhit())c=getch();
                if(c=='r'||c=='R')
                {
                    restart(sizex,sizey);break;
                }
                if(c=='e'||c=='E')
                {
                    forget_all_node(); exit(0);
                }
                if(c=='u'||c=='U')
                {
                    thisstep=step-1;
                    find=0;
                    for(x=1;x<=sizex;x++)
                    {
                        for(y=1;y<=sizey;y++)
                        {
                            if(realboard[x][y].step==thisstep)  {find=1;    break;}
                        }
                        if(find)break;
                    }
                    de_move(x,y,thisstep);
                    break;
                }
            }
            //break;
        }
        else
        {
            printf("上步：x=%d,y=%d\n",lastx,lasty);
        }
    }
}
