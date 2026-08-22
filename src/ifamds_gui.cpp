
//  IFAMDS — SFML 3.0 GUI  (Complete 1:1 Menu Mapping)
//  CL2001 Data Structures Project 2026
//
//  BUILD (MSYS2 MinGW64 + SFML 3.x)
//  
//  g++ ifamds_gui.cpp -o ifamds_gui.exe \
//      -I"C:/msys64/mingw64/include" \
//      -L"C:/msys64/mingw64/lib" \
//      -lsfml-graphics -lsfml-window -lsfml-system \
//      -mwindows -O2 -std=c++17


#include <sstream>
#include <string>
#include <vector>
#include <deque>
#include <functional>
#include <algorithm>
#include <cstring>
#include <cmath>
#include <cstdio>
#include <ctime>
#include<iostream>

using namespace std;

// Log ring buffer
static  deque< string>  g_log;
static  ostringstream       g_cap;
static  streambuf*          g_prevBuf = nullptr;
static const int LOG_MAX = 400;

static void flushLog() {
     string raw = g_cap.str();
    if (raw.empty()) return;
    g_cap.str(""); g_cap.clear();
     string ln;
    for (char c : raw) {
        if (c == '\n') {
            if ((int)g_log.size() >= LOG_MAX) g_log.pop_front();
            g_log.push_back(ln); ln.clear();
        } else ln += c;
    }
    if (!ln.empty()) {
        if ((int)g_log.size() >= LOG_MAX) g_log.pop_front();
        g_log.push_back(ln);
    }
}

static void runCap( function<void()> fn) {
    if (!g_prevBuf) g_prevBuf =  cout.rdbuf();
     cout.rdbuf(g_cap.rdbuf());
    fn();
     cout.flush();
     cout.rdbuf(g_prevBuf);
    g_prevBuf = nullptr;
    flushLog();
}

// Feed a string stream into cin, run fn with cout captured
static void runFeeding(const  string& input,  function<void()> fn) {
     istringstream iss(input);
     streambuf* ob =  cin.rdbuf(iss.rdbuf());
    runCap(fn);
     cin.rdbuf(ob);
}

// original main
#define main _orig_main_unused_
#include "dsaprojifamds.cpp"
#undef main

// ── SFML 
#include <SFML/Graphics.hpp>


//  LAYOUT & PALETTE

static const int WW = 1320, WH = 800;
static const int TOP_H  = 50;
static const int SIDE_W = 230;
static const int LOG_H  = 190;
static const int LOG_Y  = WH - LOG_H;
// content area
static const float CX = (float)SIDE_W + 6.f;
static const float CY = (float)TOP_H  + 6.f;
static const float CW = (float)(WW - SIDE_W - 12.f);
static const float CH = (float)(LOG_Y - TOP_H - 12.f);

namespace P {
    sf::Color bg      {10,17,11};
    sf::Color panel   {17,29,18};
    sf::Color panHdr  {26,46,28};
    sf::Color bdr     {36,64,40};
    sf::Color accent  {68,196,82};
    sf::Color fire    {255,108,12};
    sf::Color fireBrt {255,198,52};
    sf::Color danger  {208,42,42};
    sf::Color warn    {218,158,0};
    sf::Color smokeC  {155,155,155};
    sf::Color txt     {212,232,212};
    sf::Color txtDim  {85,125,90};
    sf::Color txtHdr  {124,212,136};
    sf::Color btnN    {22,46,26};
    sf::Color btnH    {42,84,48};
    sf::Color btnA    {56,144,66};
    sf::Color gridOk  {20,82,30};
}

//  FIRE SPARKS
struct Spark { float x,y,vx,vy,life,maxL,r; sf::Color col; };
static  vector<Spark> g_sparks;
static float g_sparkAcc = 0.f;

static void spawnSparks(float cx,float cy,int n=2){
    for(int i=0;i<n;i++){
        Spark p;
        p.x=cx+(rand()%20-10); p.y=cy;
        p.vx=((rand()%90)-45)*0.013f;
        p.vy=-((rand()%75)+30)*0.013f;
        p.life=p.maxL=(rand()%60+35)/100.f;
        p.r=(rand()%5+2)*0.45f;
        float t=(float)(rand()%100)/100.f;
        p.col={255,(uint8_t)(42+t*115),(uint8_t)(t*22),(uint8_t)200};
        g_sparks.push_back(p);
    }
}
static void tickSparks(float dt){
    for(auto& p:g_sparks){
        p.x+=p.vx*dt*60; p.y+=p.vy*dt*60; p.life-=dt*0.9f;
        p.col.a=(uint8_t) max(0.f,200.f*p.life/p.maxL);
    }
    g_sparks.erase( remove_if(g_sparks.begin(),g_sparks.end(),
        [](const Spark& p){return p.life<=0;}),g_sparks.end());
}
static void drawSparks(sf::RenderWindow& w){
    for(auto& p:g_sparks){
        sf::CircleShape c(p.r); c.setFillColor(p.col);
        c.setPosition({p.x-p.r,p.y-p.r}); w.draw(c);
    }
}

//  DRAW PRIMITIVES

static void fillRect(sf::RenderWindow& w,float x,float y,float wd,float ht,
                     sf::Color col,sf::Color bdrC={0,0,0,0},float bt=0.f){
    sf::RectangleShape r({wd,ht}); r.setPosition({x,y}); r.setFillColor(col);
    if(bt>0.f&&bdrC.a>0){r.setOutlineColor(bdrC);r.setOutlineThickness(bt);}
    w.draw(r);
}
static void hLine(sf::RenderWindow& w,float x,float y,float wd,sf::Color c){
    fillRect(w,x,y,wd,1.f,c);
}
static void lbl(sf::RenderWindow& w,const sf::Font& f,
                const  string& s,float x,float y,unsigned sz,sf::Color col){
    sf::Text t(f,s,sz); t.setFillColor(col); t.setPosition({x,y}); w.draw(t);
}
static void panelBox(sf::RenderWindow& w,const sf::Font& f,
                     float x,float y,float wd,float ht,const char* title){
    fillRect(w,x,y,wd,ht,P::panel,P::bdr,1.f);
    if(title&&title[0]){ fillRect(w,x,y,wd,20.f,P::panHdr); lbl(w,f,title,x+6,y+3,10,P::txtHdr); }
}
static void barFill(sf::RenderWindow& w,float x,float y,float wd,float ht,
                    float frac,sf::Color bg,sf::Color fg){
    fillRect(w,x,y,wd,ht,bg);
    if(frac>0.f) fillRect(w,x,y,wd* min(frac,1.f),ht,fg);
}


//  BUTTON

struct Btn {
    sf::FloatRect r;
     string lbl2;
     function<void()> cb;
    bool hov=false,act=false;

    bool hit(sf::Vector2i mp)const{ return r.contains({(float)mp.x,(float)mp.y}); }
    void draw(sf::RenderWindow& w,const sf::Font& f)const{
        sf::Color fc=act?P::btnA:(hov?P::btnH:P::btnN);
        sf::Color bc=act?P::accent:(hov?P::txtHdr:P::bdr);
        fillRect(w,r.position.x,r.position.y,r.size.x,r.size.y,fc,bc,1.3f);
        sf::Text t(f,lbl2,12); t.setFillColor(act?P::bg:(hov?P::txt:P::txtDim));
        auto tb=t.getLocalBounds();
        t.setPosition({r.position.x+(r.size.x-tb.size.x)/2.f-tb.position.x,
                       r.position.y+(r.size.y-tb.size.y)/2.f-tb.position.y});
        w.draw(t);
    }
};


//  INPUT DIALOG
//  Supports up to 4 labelled fields. On confirm, calls cb with
//  a newline-separated string of all entered values.

struct InputField {  string label, buf, placeholder; };

struct Dialog {
    bool        active = false;
     string title;
     vector<InputField> fields;
    int         focused = 0;          // which field is active
     function<void( vector< string>)> onOk;
     function<void()>                          onCancel;
};
static Dialog g_dlg;

static void showDialog(const  string& title,
                        vector<InputField> fields,
                        function<void( vector< string>)> onOk){
    g_dlg.title   = title;
    g_dlg.fields  =  move(fields);
    g_dlg.focused = 0;
    g_dlg.onOk    =  move(onOk);
    g_dlg.active  = true;
}

static void drawDialog(sf::RenderWindow& w,const sf::Font& f){
    if(!g_dlg.active) return;
    // dim overlay
    sf::RectangleShape ov({(float)WW,(float)WH}); ov.setFillColor({0,0,0,165}); w.draw(ov);

    int n = (int)g_dlg.fields.size();
    float dh = 80.f + n*52.f;
    float dw = 480.f;
    float dx = (WW-dw)/2.f, dy = (WH-dh)/2.f;

    fillRect(w,dx,dy,dw,dh,P::panel,P::accent,2.f);
    fillRect(w,dx,dy,dw,24.f,P::panHdr);
    lbl(w,f,g_dlg.title,dx+8.f,dy+4.f,13,P::txtHdr);

    for(int i=0;i<n;i++){
        float fy=dy+28.f+i*52.f;
        lbl(w,f,g_dlg.fields[i].label,dx+14.f,fy+4.f,12,P::txt);
        bool foc=(i==g_dlg.focused);
        fillRect(w,dx+14.f,fy+20.f,dw-28.f,26.f,{14,24,15},foc?P::accent:P::bdr,foc?2.f:1.f);
         string shown = g_dlg.fields[i].buf.empty()
                            ? g_dlg.fields[i].placeholder
                            : g_dlg.fields[i].buf + (foc?"|":"");
        sf::Color tc = g_dlg.fields[i].buf.empty() ? P::txtDim : P::txt;
        lbl(w,f,shown,dx+18.f,fy+23.f,13,tc);
    }

    // OK / Cancel
    float by=dy+dh-36.f;
    fillRect(w,dx+14.f,   by,100.f,28.f,P::btnA,P::accent,1.f);
    lbl(w,f,"OK (Enter)",dx+22.f,by+7.f,12,P::bg);
    fillRect(w,dx+124.f,  by,100.f,28.f,P::btnN,P::bdr,1.f);
    lbl(w,f,"Cancel (Esc)",dx+128.f,by+7.f,12,P::txtDim);
    lbl(w,f,"Tab = next field",dx+250.f,by+8.f,11,P::txtDim);
}

static void dlgKeyPressed(sf::Keyboard::Key k){
    if(!g_dlg.active) return;
    int n=(int)g_dlg.fields.size();
    if(k==sf::Keyboard::Key::Escape){
        g_dlg.active=false;
    } else if(k==sf::Keyboard::Key::Enter){
        // validate non-empty (allow placeholder pass-through)
         vector< string> vals;
        for(auto& fi:g_dlg.fields){
            vals.push_back(fi.buf.empty()?fi.placeholder:fi.buf);
        }
        g_dlg.active=false;
        if(g_dlg.onOk) g_dlg.onOk(vals);
    } else if(k==sf::Keyboard::Key::Tab){
        g_dlg.focused=(g_dlg.focused+1)%n;
    } else if(k==sf::Keyboard::Key::Backspace){
        auto& buf=g_dlg.fields[g_dlg.focused].buf;
        if(!buf.empty()) buf.pop_back();
    }
}
static void dlgTextEntered(uint32_t unicode){
    if(!g_dlg.active) return;
    if(unicode>=32&&unicode<127)
        g_dlg.fields[g_dlg.focused].buf+=(char)unicode;
}


//  SCREEN ENUM  (SPLASH = intro screen before dashboard)

enum class Scr { SPLASH, MAIN, M1,M2,M3,M4,M5,M6,M7,M8,M9,M10 };
static Scr g_scr = Scr::SPLASH;


//  GLOBAL BUTTON LIST (rebuilt each frame per screen)

static  vector<Btn> g_btns;
static  vector<Btn> g_navBtns;


//  HELPERS

static float g_logScroll = 0.f;

static void logLine(const  string& s){
    if((int)g_log.size()>=LOG_MAX) g_log.pop_front();
    g_log.push_back(s);
}

static void drawTopBar(sf::RenderWindow& w,const sf::Font& f){
    fillRect(w,0,0,(float)WW,(float)TOP_H,{8,14,9},P::bdr,1.f);
    lbl(w,f,"IFAMDS",12.f,11.f,22,P::accent);
    lbl(w,f,"Intelligent Forest Advisory & Multi-Structure Decision System",106.f,17.f,11,P::txtDim);
    fillRect(w,0.f,(float)TOP_H-1.5f,(float)WW,1.5f,P::accent);
}

static void drawLogPanel(sf::RenderWindow& w,const sf::Font& f){
    float x=(float)SIDE_W, y=(float)LOG_Y, wd=(float)(WW-SIDE_W), ht=(float)LOG_H;
    panelBox(w,f,x,y,wd,ht,"OUTPUT LOG  (mousewheel to scroll)");
    const int LH=14; int vis=(int)((ht-24)/LH);
    int total=(int)g_log.size();
    int start= max(0,(int)(total-vis-g_logScroll));
    int end= min(total,start+vis);
    float ty=y+22.f;
    for(int i=start;i<end;i++){
        const  string& ln=g_log[i];
        sf::Color col=P::txt;
        if(ln.find("ANOMALY")!=ln.npos||ln.find("FIRE")!=ln.npos||
           ln.find("ALERT")!=ln.npos||ln.find("EMERGENCY")!=ln.npos) col=P::fire;
        else if(ln.find("WARN")!=ln.npos)   col=P::warn;
        else if(ln.find("RESTORE")!=ln.npos||
                ln.find("Complete")!=ln.npos)col=P::accent;
        else if(!ln.empty()&&ln[0]=='[')     col=P::txtHdr;
        lbl(w,f,ln.substr(0,132),x+5.f,ty,11,col); ty+=LH;
    }
}


//  SIDEBAR  (main navigation)

struct NavEntry { const char* label; Scr sc; };
static const NavEntry NAV[]={
    {"MAIN MENU",    Scr::MAIN},
    {"1. Input Env Data",    Scr::M1},
    {"2. Forest Grid",       Scr::M2},
    {"3. Event Memory",      Scr::M3},
    {"4. Fire Detection",    Scr::M4},
    {"5. Task Scheduling",   Scr::M5},
    {"6. Decision System",   Scr::M6},
    {"7. Spatial Routing",   Scr::M7},
    {"8. Hash Access",       Scr::M8},
    {"9. System Monitor",    Scr::M9},
    {"10. Scenarios",        Scr::M10},
};
static const int NCOUNT=11;

static void buildNav(){
    g_navBtns.clear();
    float y=(float)TOP_H+8.f;
    for(int i=0;i<NCOUNT;i++){
        Btn b; b.r={{4.f,y},{(float)SIDE_W-8.f,38.f}};
        b.lbl2=NAV[i].label; b.act=(g_scr==NAV[i].sc);
        Scr sc=NAV[i].sc; b.cb=[sc]{g_scr=sc; g_btns.clear();};
        g_navBtns.push_back(b); y+=44.f;
    }
}
static void drawSidebar(sf::RenderWindow& w,const sf::Font& f){
    fillRect(w,0.f,(float)TOP_H,(float)SIDE_W,(float)(WH-TOP_H),{8,14,9});
    fillRect(w,(float)SIDE_W-1.5f,(float)TOP_H,1.5f,(float)(WH-TOP_H),P::bdr);
    lbl(w,f,"NAVIGATION",5.f,(float)TOP_H+1.f,9,P::txtDim);
    buildNav();
    for(auto& b:g_navBtns) b.draw(w,f);
}


//  SUBMENU BUTTON BUILDER HLPER

struct SubItem {  string label;  function<void()> cb; };

static void buildSubItems(const  string& menuTitle,
                           const  vector<SubItem>& items,
                           sf::RenderWindow& w, const sf::Font& f){
    // Draw menu title
    lbl(w,f,menuTitle,CX,CY,17,P::txtHdr);

    g_btns.clear();
    float bx=CX, by=CY+32.f;
    const float BW=286.f, BH=38.f, GAP_X=6.f, GAP_Y=6.f;
    const int COLS=3;
    int col=0;
    for(int i=0;i<(int)items.size();i++){
        Btn b; b.r={{bx,by},{BW,BH}}; b.lbl2=items[i].label;
        auto fn=items[i].cb; b.cb=fn;
        g_btns.push_back(b);
        col++;
        if(col==COLS){ bx=CX; by+=BH+GAP_Y; col=0; }
        else          bx+=BW+GAP_X;
    }

    // Back button
    Btn back; back.r={{CX,(float)LOG_Y-46.f},{120.f,36.f}};
    back.lbl2="← Back"; back.cb=[]{ g_scr=Scr::MAIN; g_btns.clear(); };
    g_btns.push_back(back);
}


//  SPLASH SCREEN
//  Fullscreen cinematic intro. Inspired by the IFAMDS image:
//  deep forest background, glowing green title, amber fire
//  accent, animated data-node ring, and pulsing ENTER prompt.

static void drawSplash(sf::RenderWindow& w, const sf::Font& f, float t) {
    // ── 1. Dark forest background gradient (top = deep green,
    //        bottom = near-black) ─────
    for (int y = 0; y < WH; y++) {
        float frac = (float)y / WH;
        uint8_t r = (uint8_t)(4  + frac * 6);
        uint8_t g2= (uint8_t)(12 + (1.f - frac) * 18);
        uint8_t b = (uint8_t)(4  + frac * 6);
        sf::RectangleShape sl({(float)WW, 1.f});
        sl.setPosition({0.f, (float)y});
        sl.setFillColor({r, g2, b});
        w.draw(sl);
    }

    // ── 2. Animated glowing grid / wireframe forest floor ────
    //   Draws a perspective-like grid fading from centre-bottom
    const int GLINES = 8;
    float vanishX = WW * 0.5f, vanishY = WH * 0.42f;
    sf::Color gridCol{38, 120, 48, 80};
    for (int i = 0; i <= GLINES; i++) {
        float t2 = (float)i / GLINES;
        float bx = 60.f + t2 * (WW - 120.f);
        sf::Vertex vl[2] = {{sf::Vector2f(bx, (float)WH), gridCol},
                             {sf::Vector2f(vanishX, vanishY), gridCol}};
        w.draw(vl, 2, sf::PrimitiveType::Lines);
    }
    for (int i = 1; i <= 6; i++) {
        float yf   = vanishY + (WH - vanishY) * ((float)i / 6.f);
        float xspan= 60.f + (WW - 120.f) * (1.f - (float)i / 6.f) * 0.5f;
        float xl   = vanishX - xspan * 0.5f;
        float xr   = vanishX + xspan * 0.5f;
        sf::Vertex hl[2] = {{sf::Vector2f(xl, yf), gridCol},
                             {sf::Vector2f(xr, yf), gridCol}};
        w.draw(hl, 2, sf::PrimitiveType::Lines);
    }

    // 3. Animated data-node ring (centre of screen)
    const int NODES = 10;
    float ringCX = WW * 0.5f, ringCY = WH * 0.52f;
    float ringR  = 80.f + 6.f *  sin(t * 1.2f);
    sf::Vector2f nodePos[NODES];
    for (int i = 0; i < NODES; i++) {
        float a = (float)i / NODES * 2.f * 3.14159f - 1.57f + t * 0.3f;
        nodePos[i] = {ringCX + ringR *  cos(a),
                      ringCY + ringR *  sin(a)};
    }
    // edges
    int edges[][2] = {{0,1},{1,2},{2,3},{3,4},{4,5},{5,6},{6,7},{7,8},{8,9},{9,0},
                      {0,5},{1,6},{2,7},{3,8}};
    for (auto& e : edges) {
        float pulse = 0.4f + 0.3f *  sin(t * 2.f + e[0]);
        sf::Color ec{(uint8_t)(36*pulse*3), (uint8_t)(180*pulse), (uint8_t)(48*pulse),
                     (uint8_t)(120*pulse)};
        sf::Vertex el[2] = {{nodePos[e[0]], ec}, {nodePos[e[1]], ec}};
        w.draw(el, 2, sf::PrimitiveType::Lines);
    }
    // nodes
    for (int i = 0; i < NODES; i++) {
        float pulse = 0.6f + 0.4f *  sin(t * 2.5f + i * 0.7f);
        sf::CircleShape c(5.f * pulse);
        c.setOrigin({5.f * pulse, 5.f * pulse});
        c.setPosition(nodePos[i]);
        c.setFillColor({(uint8_t)(60*pulse), (uint8_t)(210*pulse), (uint8_t)(72*pulse),
                        (uint8_t)(220*pulse)});
        w.draw(c);
        // fire sparks on a couple of nodes
        if (i == 3 || i == 7) spawnSparks(nodePos[i].x, nodePos[i].y, 1);
    }

    // 4. Left HUD panel — Decision Sys 
    float lpx = 60.f, lpy = 155.f, lpw = 310.f, lph = 290.f;
    {
        sf::RectangleShape pn({lpw, lph}); pn.setPosition({lpx, lpy});
        pn.setFillColor({14, 38, 18, 180});
        pn.setOutlineColor({48, 200, 64, 200}); pn.setOutlineThickness(1.5f);
        w.draw(pn);
        lbl(w, f, "DECISION SYS", lpx + 14.f, lpy + 12.f, 18, {68, 210, 80});
        // mini tree lines
        float tx = lpx + 155.f, ty2 = lpy + 54.f;
        float nodeR = 8.f;
        // 3 levels: 1 root, 2 mid, 4 leaves
        sf::Vector2f tpos[] = {
            {tx, ty2},
            {tx - 55.f, ty2 + 52.f}, {tx + 55.f, ty2 + 52.f},
            {tx - 82.f, ty2 + 104.f}, {tx - 27.f, ty2 + 104.f},
            {tx + 27.f, ty2 + 104.f}, {tx + 82.f, ty2 + 104.f},
        };
        int tEdges[][2] = {{0,1},{0,2},{1,3},{1,4},{2,5},{2,6}};
        sf::Color tec{48, 180, 60, 200};
        for (auto& e : tEdges) {
            sf::Vertex el[2] = {{tpos[e[0]], tec}, {tpos[e[1]], tec}};
            w.draw(el, 2, sf::PrimitiveType::Lines);
        }
        for (auto& p : tpos) {
            sf::CircleShape c(nodeR); c.setOrigin({nodeR, nodeR}); c.setPosition(p);
            c.setFillColor({24, 60, 28}); c.setOutlineColor({68, 210, 80, 220});
            c.setOutlineThickness(1.5f); w.draw(c);
        }
        lbl(w, f, "RISK SCORES", lpx + 12.f, lpy + 220.f, 12, {68, 210, 80});
        lbl(w, f, "MULTI-STRUCTURE ANALYSIS", lpx + 12.f, lpy + 238.f, 12, {68, 210, 80});
    }

    // ── 5. Right HUD panel — Fire Detection 
    float rpx = (float)WW - 380.f, rpy = 155.f, rpw = 310.f, rph = 290.f;
    {
        sf::RectangleShape pn({rpw, rph}); pn.setPosition({rpx, rpy});
        pn.setFillColor({38, 18, 6, 180});
        float amber = 0.7f + 0.3f *  sin(t * 2.8f);
        pn.setOutlineColor({(uint8_t)(220*amber), (uint8_t)(140*amber), 0, 210});
        pn.setOutlineThickness(1.5f);
        w.draw(pn);
        sf::Color fc{(uint8_t)(220 + 35* sin(t*3.f)), (uint8_t)(108+40* sin(t*2.f)), 12};
        lbl(w, f, "FIRE DETECTION", rpx + 14.f, rpy + 12.f, 18, fc);

        // Animated flame shape using circles
        float fx = rpx + 155.f, fy = rpy + 145.f;
        float fl = 0.6f + 0.4f *  sin(t * 4.f);
        for (int ring = 0; ring < 4; ring++) {
            float ri = (4 - ring) * 18.f * fl;
            float alpha = 180.f - ring * 40.f;
            sf::CircleShape fc2(ri);
            fc2.setOrigin({ri, ri});
            fc2.setPosition({fx, fy - ring * 12.f * fl});
            uint8_t rr = (uint8_t)(255 - ring * 20);
            uint8_t gg = (uint8_t)( max(0, 80 - ring * 25));
            fc2.setFillColor({rr, gg, 0, (uint8_t)alpha});
            w.draw(fc2);
        }
        // inner bright core
        sf::CircleShape core(10.f * fl); core.setOrigin({10.f*fl, 10.f*fl});
        core.setPosition({fx, fy - 24.f * fl});
        core.setFillColor({255, 220, 80, 200}); w.draw(core);

        lbl(w, f, "THRESHOLD BFS SPREAD", rpx + 12.f, rpy + 228.f, 12, {220, 160, 0});
        lbl(w, f, "RESOURCE STATUS", rpx + 12.f, rpy + 246.f, 12, {220, 160, 0});
    }

    // ── 6. Centre label — FOREST GRID 
    {
        float fgx = WW * 0.5f;
        lbl(w, f, "FOREST GRID", fgx - 72.f, vanishY - 55.f, 18, {68, 200, 76});
        // small 3D grid icon at vanish point
        float gx = fgx - 30.f, gy2 = vanishY - 30.f;
        for (int gi = 0; gi <= 3; gi++) {
            float gp = gi * 20.f;
            sf::Color gc2{38, 160, 52, (uint8_t)(100 + gi * 30)};
            sf::Vertex h2[2] = {{sf::Vector2f(gx, gy2 + gp), gc2},
                                 {sf::Vector2f(gx + 60.f, gy2 + gp), gc2}};
            sf::Vertex v2[2] = {{sf::Vector2f(gx + gp, gy2), gc2},
                                 {sf::Vector2f(gx + gp, gy2 + 60.f), gc2}};
            w.draw(h2, 2, sf::PrimitiveType::Lines);
            w.draw(v2, 2, sf::PrimitiveType::Lines);
        }
    }

    // ── 7. IFAMDS main title
    {
        float pulse = 0.85f + 0.15f *  sin(t * 1.5f);
        // glow shadow layers
        for (int g2 = 3; g2 >= 1; g2--) {
            sf::Text glow(f, "IFAMDS", 88);
            glow.setFillColor({(uint8_t)(20*g2), (uint8_t)(140*g2*pulse), (uint8_t)(24*g2),
                               (uint8_t)(40 * g2)});
            auto tb = glow.getLocalBounds();
            glow.setPosition({(WW - tb.size.x) / 2.f - tb.position.x + g2 * 2.f,
                              18.f + g2 * 2.f});
            w.draw(glow);
        }
        sf::Text title(f, "IFAMDS", 88);
        title.setFillColor({(uint8_t)(52*pulse), (uint8_t)(210*pulse), (uint8_t)(64*pulse)});
        auto tb = title.getLocalBounds();
        title.setPosition({(WW - tb.size.x) / 2.f - tb.position.x, 18.f});
        w.draw(title);

        // subtitle
        sf::Text sub(f, "INTELLIGENT FOREST ADVISORY & MULTI-STRUCTURE DECISION SYSTEM",
                     13);
        sub.setFillColor({100, 200, 112, (uint8_t)(200 * pulse)});
        auto sb = sub.getLocalBounds();
        sub.setPosition({(WW - sb.size.x) / 2.f - sb.position.x, 114.f});
        w.draw(sub);
    }

    // ── 8. Live zone badges strip 
    {
        lbl(w, f, "LIVE ZONE STATUS:", (float)SIDE_W + 10.f, (float)WH - 210.f, 11,
            {80, 200, 90});
        float zx = (float)SIDE_W + 10.f;
        float zy = (float)WH - 192.f;
        for (int z = 0; z < MAX_ZONES && zx + 92.f < (float)WW; z++) {
            int idx = sensorData.latestIdx(z);
            float temp  = (idx >= 0) ? sensorData.temperature[z][idx] : 0.f;
            float smk   = (idx >= 0) ? sensorData.smoke[z][idx]       : 0.f;
            bool onF = (temp > TEMP_THRESHOLD || smk > SMOKE_THRESHOLD);
            fillRect(w, zx, zy, 88.f, 62.f,
                     onF ? sf::Color{52, 12, 4, 200} : sf::Color{14, 30, 16, 200},
                     onF ? P::fire : sf::Color{40, 100, 48, 180},
                     onF ? 2.f : 1.f);
            lbl(w, f, "Z" +  to_string(z), zx + 4.f, zy + 3.f, 11,
                onF ? P::fireBrt : P::txtHdr);
            if (idx >= 0) {
                char buf[20]; snprintf(buf, 20, "T:%.0f S:%.0f", temp, smk);
                lbl(w, f, buf, zx + 4.f, zy + 18.f, 9, onF ? P::fire : P::txt);
                char buf2[14]; snprintf(buf2, 14, "H:%.0f%%",
                                        sensorData.humidity[z][idx]);
                lbl(w, f, buf2, zx + 4.f, zy + 32.f, 9, P::txtDim);
            } else {
                lbl(w, f, "", zx + 4.f, zy + 24.f, 9, P::txtDim);
            }
            zx += 96.f;
        }
    }

    // ── 9. Pulsing ENTER prompt 
    {
        float alpha = 140.f + 115.f *  sin(t * 2.2f);
        sf::Text prompt(f, "PRESS  ENTER  TO  LAUNCH  SYSTEM", 16);
        prompt.setFillColor({68, 210, 80, (uint8_t)alpha});
        auto pb = prompt.getLocalBounds();
        prompt.setPosition({(WW - pb.size.x) / 2.f - pb.position.x,
                            (float)WH - 42.f});
        w.draw(prompt);

        // decorative line either side of prompt
        float pw = pb.size.x + 60.f;
        float px = (WW - pw) / 2.f;
        float py2 = (float)WH - 28.f;
        sf::Color lc{68, 210, 80, (uint8_t)(alpha * 0.6f)};
        fillRect(w, px, py2, (pw - pb.size.x - 10.f) / 2.f, 1.f, lc);
        fillRect(w, (WW + pb.size.x + 10.f) / 2.f, py2,
                 (pw - pb.size.x - 10.f) / 2.f, 1.f, lc);
    }
}


//  MAIN MENU SCREEN

static void drawMainMenu(sf::RenderWindow& w,const sf::Font& f,float t){
    lbl(w,f,"IFAMDS  —  MAIN MENU",CX,CY,18,P::txtHdr);
    lbl(w,f,"Select a module from the sidebar or click a tile below.",CX,CY+26.f,11,P::txtDim);

    struct Tile { const char* num; const char* name; const char* desc; Scr sc; sf::Color col; };
    Tile tiles[]={
        {"1","Input Env Data","Add readings, validate, compare",    Scr::M1, P::accent},
        {"2","Forest Grid",   "1D series, 2D matrix, zone view",    Scr::M2, P::accent},
        {"3","Event Memory",  "Linked lists L1-L10, traversal",     Scr::M3, P::txtHdr},
        {"4","Fire Detection","Threshold, BFS spread, resources",   Scr::M4, P::fire},
        {"5","Task Schedule", "Queues Q1-Q4, pause/resume",         Scr::M5, P::warn},
        {"6","Decision Sys",  "Risk scores, trees T10-T12",         Scr::M6, P::txtHdr},
        {"7","Spatial Route", "Graph BFS/DFS, safe path",           Scr::M7, P::accent},
        {"8","Hash Access",   "H1 insert/lookup, H2 chain, H3 cache",Scr::M8,P::accent},
        {"9","Sys Monitor",   "Load, latency, bottleneck, optimize",Scr::M9, P::warn},
        {"10","Scenarios",    "Run all 5 complete test scenarios",  Scr::M10,P::fireBrt},
    };

    g_btns.clear();
    float tx=CX, ty=CY+46.f;
    const float TW=200.f, TH=80.f;
    int col=0;
    for(auto& tile:tiles){
        fillRect(w,tx,ty,TW,TH,P::panel,tile.col,1.f);
        lbl(w,f,tile.num,tx+6.f,ty+5.f,13,tile.col);
        lbl(w,f,tile.name,tx+30.f,ty+5.f,13,P::txt);
        lbl(w,f,tile.desc,tx+6.f,ty+26.f,10,P::txtDim);
        Btn b; b.r={{tx,ty},{TW,TH}}; b.lbl2="";
        Scr sc=tile.sc; b.cb=[sc]{g_scr=sc; g_btns.clear();};
        g_btns.push_back(b);
        col++; tx+=TW+5.f;
        if(col==5){ col=0; tx=CX; ty+=TH+5.f; }
    }

    // Zone status strip
    float sy=ty+(col>0?TH+5.f:0.f)+8.f;
    if(sy<CY+220.f) sy=CY+220.f;
    lbl(w,f,"Live Zone Status:",CX,sy,12,P::txtHdr);
    float zx=CX; sy+=16.f;
    for(int z=0;z<MAX_ZONES;z++){
        int idx=sensorData.latestIdx(z);
        float temp=(idx>=0)?sensorData.temperature[z][idx]:0.f;
        float smk =(idx>=0)?sensorData.smoke[z][idx]:0.f;
        bool onF=(temp>TEMP_THRESHOLD||smk>SMOKE_THRESHOLD);
        sf::Color bc=onF?P::fire:P::bdr;
        fillRect(w,zx,sy,90.f,54.f,onF?sf::Color{45,10,4}:P::panel,bc,onF?2.f:1.f);
        lbl(w,f,"Z"+ to_string(z),zx+4.f,sy+3.f,10,onF?P::fireBrt:P::txtHdr);
        if(idx>=0){
            char buf[20]; snprintf(buf,20,"T:%.0f S:%.0f",temp,smk);
            lbl(w,f,buf,zx+4.f,sy+18.f,9,onF?P::fire:P::txt);
            char buf2[12]; snprintf(buf2,12,"H:%.0f%%",sensorData.humidity[z][idx]);
            lbl(w,f,buf2,zx+4.f,sy+32.f,9,P::txtDim);
        } else { lbl(w,f,"(empty)",zx+4.f,sy+22.f,9,P::txtDim); }
        if(onF) spawnSparks(zx+45.f,sy+2.f,1);
        zx+=96.f;
    }
}


//  MENU 1 — Input Environmental Data

static void renderM1(sf::RenderWindow& w,const sf::Font& f){
     vector<SubItem> items={
        // 1.1 Add Sensor Reading
        {"1.1  Add Sensor Reading", []{
            showDialog("1.1  Add Sensor Reading",{
                {"Zone (0-9):","","0"},{"Temperature (C):","","25"},
                {"Smoke (ppm):","","5"},{"Humidity (%):","","60"}
            },[]( vector< string> v){
                 string inp=v[0]+"\n"+v[1]+"\n"+v[2]+"\n"+v[3]+"\n";
                runFeeding(inp,[]{
                    int zone; float temp,smoke,humidity;
                     cin>>zone>>temp>>smoke>>humidity;
                    bool ok=sensorData.addReading(zone,temp,smoke,humidity);
                    if(!ok){ cout<<"[ERROR] Zone out of range or array full.\n";return;}
                     cout<<"[A2] Reading stored in dynamic array.\n";
                    int row=zone/GRID_COLS,col=zone%GRID_COLS;
                    dynamicGrid.setCell(row,col,temp,smoke,humidity);
                    rollbackStack.push(dynamicGrid,"pre-reading");
                     cout<<"[L-LAYER] Routing through event memory...\n";
                    ingestEvent(zone,temp,"TEMP",BASELINE_TEMP[zone]);
                    ingestEvent(zone,smoke,"SMOKE",BASELINE_SMOKE[zone]);
                    ingestEvent(zone,humidity,"HUMIDITY",BASELINE_HUMIDITY[zone]);
                });
            });
        }},
        // 1.2 Store Data in Dynamic Array
        {"1.2  Store / View Dynamic Array", []{
            runCap([]{
                 cout<<"[A2] Dynamic Sensor Array contents:\n";
                bool any=false;
                for(int z=0;z<MAX_ZONES;z++){
                    if(sensorData.count[z]==0)continue; any=true;
                     cout<<"  Zone "<<z<<"  ("<<sensorData.count[z]<<" readings):\n";
                    for(int r=0;r<sensorData.count[z];r++)
                         cout<<"    ["<<r<<"]  T="<<sensorData.temperature[z][r]
                                 <<"  S="<<sensorData.smoke[z][r]
                                 <<"  H="<<sensorData.humidity[z][r]<<"\n";
                }
                if(!any) cout<<"  (no data yet)\n";
            });
        }},
        // 1.3 Compare with Static Baseline
        {"1.3  Compare vs Static Baseline", []{
            runCap([]{
                 cout<<"[A1 vs A2] Baseline Comparison:\n";
                 cout<<"  Zone | dTemp | dSmoke | dHumid | Status\n";
                bool any=false;
                for(int z=0;z<MAX_ZONES;z++){
                    if(sensorData.count[z]==0)continue; any=true;
                    int i=sensorData.latestIdx(z);
                    float dT=sensorData.temperature[z][i]-BASELINE_TEMP[z];
                    float dS=sensorData.smoke[z][i]-BASELINE_SMOKE[z];
                    float dH=sensorData.humidity[z][i]-BASELINE_HUMIDITY[z];
                    bool alert=(sensorData.temperature[z][i]>TEMP_THRESHOLD||
                                sensorData.smoke[z][i]>SMOKE_THRESHOLD||
                                sensorData.humidity[z][i]<HUMIDITY_THRESHOLD);
                     cout<<"   "<<z<<"   |  "<<dT<<"  |  "<<dS<<"  |  "<<dH
                             <<"  |  "<<(alert?"ALERT":"OK")<<"\n";
                }
                if(!any) cout<<"  (no data yet)\n";
            });
        }},
        // 1.4 Validate and Filter Noise
        {"1.4  Validate and Filter Noise", []{
            runCap([]{
                 cout<<"[FILTER] Noise / Anomaly Detection Report:\n";
                bool any=false;
                for(int z=0;z<MAX_ZONES;z++){
                    int cnt=sensorData.count[z]; if(cnt==0)continue; any=true;
                     cout<<"  Zone "<<z<<":\n";
                    for(int r=0;r<cnt;r++){
                        float t=sensorData.temperature[z][r];
                        float s=sensorData.smoke[z][r];
                        float h=sensorData.humidity[z][r];
                         cout<<"    ["<<r<<"]  T="<<t<<(isAnomaly(t,BASELINE_TEMP[z])?"(!)":"   ")
                                 <<"  S="<<s<<(isAnomaly(s,BASELINE_SMOKE[z])?"(!)":"   ")
                                 <<"  H="<<h<<(isAnomaly(h,BASELINE_HUMIDITY[z])?"(!)":"   ")<<"\n";
                    }
                }
                if(!any) cout<<"  (no data yet)\n";
            });
        }},
    };
    buildSubItems("1. Input Environmental Data",items,w,f);
    for(auto& b:g_btns) b.draw(w,f);
}


//  MENU 2 — Forest Grid Status

static void renderM2(sf::RenderWindow& w,const sf::Font& f,float t){
    // Draw the 2D grids visually above the buttons
    lbl(w,f,"2. View Forest Grid Status",CX,CY,17,P::txtHdr);

    float cw=50.f,ch=26.f;
    panelBox(w,f,CX,CY+26.f,320.f,180.f,"A3  STATIC BASELINE (Temp C)");
    panelBox(w,f,CX+328.f,CY+26.f,320.f,180.f,"A4  DYNAMIC LIVE GRID (Temp C)");
    for(int r=0;r<GRID_ROWS;r++) for(int c2=0;c2<GRID_COLS;c2++){
        if(r==0){ char cb[4];snprintf(cb,4,"C%d",c2);
            lbl(w,f,cb,CX+14.f+c2*cw+14.f,CY+38.f,8,P::txtDim);
            lbl(w,f,cb,CX+342.f+c2*cw+14.f,CY+38.f,8,P::txtDim);}
        if(c2==0){ char rb[4];snprintf(rb,4,"R%d",r);
            lbl(w,f,rb,CX+2.f,CY+50.f+r*ch,8,P::txtDim);
            lbl(w,f,rb,CX+330.f,CY+50.f+r*ch,8,P::txtDim);}
        // static
        fillRect(w,CX+12.f+c2*cw,CY+48.f+r*ch,cw-1.f,ch-1.f,P::gridOk);
        char sv[8];snprintf(sv,8,"%.0f",staticGrid.temperature[r][c2]);
        lbl(w,f,sv,CX+16.f+c2*cw,CY+51.f+r*ch,10,P::txt);
        // dynamic
        float dt=dynamicGrid.temperature[r][c2];
        float ds=dynamicGrid.smoke[r][c2];
        bool fire=(dt>TEMP_THRESHOLD||ds>SMOKE_THRESHOLD);
        sf::Color dc=fire?sf::Color{(uint8_t)(155+45* sin(t*4+r+c2)),36,4}:
                    dt>35.f?sf::Color{115,68,8}:P::gridOk;
        fillRect(w,CX+340.f+c2*cw,CY+48.f+r*ch,cw-1.f,ch-1.f,dc,fire?P::fire:P::bdr,fire?1.5f:.4f);
        char dv[8];snprintf(dv,8,"%.0f",dt);
        lbl(w,f,dv,CX+344.f+c2*cw,CY+51.f+r*ch,10,fire?P::fireBrt:P::txt);
        if(fire) spawnSparks(CX+340.f+c2*cw+cw/2,CY+48.f+r*ch,1);
    }

    g_btns.clear();
    float by=CY+218.f;

     vector<SubItem> items={
        {"2.1  1D Time Series (pick zone)",[]{
            showDialog("2.1  Display 1D Time Series",{{"Zone (0-9):","","0"}},
            []( vector< string> v){
                runFeeding(v[0]+"\n",[]{
                    int zone;  cin>>zone;
                    int cnt=sensorData.count[zone];
                    if(cnt==0){ cout<<"  No data for zone "<<zone<<"\n";return;}
                     cout<<"[A2] 1D Time Series - Zone "<<zone<<" ("<<cnt<<" readings):\n";
                     cout<<"  Idx | Temp  | Smoke | Humid\n";
                    for(int r=0;r<cnt;r++)
                         cout<<"  "<<r<<" | "<<sensorData.temperature[zone][r]
                                 <<" | "<<sensorData.smoke[zone][r]
                                 <<" | "<<sensorData.humidity[zone][r]<<"\n";
                    if(cnt>=2){float fi=sensorData.temperature[zone][0],la=sensorData.temperature[zone][cnt-1];
                         cout<<"  Trend: "<<(la>fi?"RISING":la<fi?"FALLING":"STABLE")<<"\n";}
                });
            });
        }},
        {"2.2  Display 2D Grid Matrix",[]{
            runCap([]{
                 cout<<"[A3] STATIC Baseline Grid (Temp):\n  ";
                for(int c=0;c<GRID_COLS;c++) cout<<"  C"<<c;
                 cout<<"\n";
                for(int r=0;r<GRID_ROWS;r++){ cout<<"  R"<<r;
                    for(int c=0;c<GRID_COLS;c++) cout<<"  "<<staticGrid.temperature[r][c];
                     cout<<"\n";}
                 cout<<"\n[A4] DYNAMIC Live Grid (Temp):\n  ";
                for(int c=0;c<GRID_COLS;c++) cout<<"  C"<<c;
                 cout<<"\n";
                for(int r=0;r<GRID_ROWS;r++){ cout<<"  R"<<r;
                    for(int c=0;c<GRID_COLS;c++){float tt=dynamicGrid.temperature[r][c];
                         cout<<(tt>TEMP_THRESHOLD?"[!]":"   ")<<tt;}
                     cout<<"\n";}
                 cout<<"\n[BOUNDARY] Sharp boundaries:\n"; bool found=false;
                for(int r=0;r<GRID_ROWS;r++) for(int c=0;c<GRID_COLS;c++){
                    if(c+1<GRID_COLS&&isBoundary(dynamicGrid.temperature[r][c],dynamicGrid.temperature[r][c+1])){
                         cout<<"  R"<<r<<"C"<<c<<" <-> R"<<r<<"C"<<c+1<<"\n";found=true;}
                }
                if(!found) cout<<"  None detected.\n";
            });
        }},
        {"2.3  Zone-wise Conditions",[]{
            runCap([]{
                 cout<<"  Zone | Temp    | Smoke   | Humid  | Status\n";
                 cout<<"  -----|---------|---------|--------|--------\n";
                for(int z=0;z<MAX_ZONES;z++){
                    int i=sensorData.latestIdx(z);
                    if(i<0){ cout<<"   "<<z<<"   | (no data)\n";continue;}
                    float t=sensorData.temperature[z][i],s=sensorData.smoke[z][i],h=sensorData.humidity[z][i];
                    const char* st=(t>TEMP_THRESHOLD||s>SMOKE_THRESHOLD)?"FIRE RISK":(h<HUMIDITY_THRESHOLD?"DRY":"NORMAL");
                     cout<<"   "<<z<<"   | "<<t<<" | "<<s<<" | "<<h<<" | "<<st<<"\n";
                }
            });
        }},
    };

    float bx=CX; float btn_y=by;
    const float BW=290.f,BH=38.f;
    for(int i=0;i<(int)items.size();i++){
        Btn b; b.r={{bx,btn_y},{BW,BH}}; b.lbl2=items[i].label;
        auto fn=items[i].cb; b.cb=fn; g_btns.push_back(b);
        bx+=BW+6.f;
    }
    Btn back; back.r={{CX,(float)LOG_Y-46.f},{120.f,36.f}};
    back.lbl2="← Back"; back.cb=[]{ g_scr=Scr::MAIN; g_btns.clear(); };
    g_btns.push_back(back);
    for(auto& b:g_btns) b.draw(w,f);
}


//  MENU 3 — Event Memory System

static void renderM3(sf::RenderWindow& w,const sf::Font& f){
    // Show linked list visualizer
    lbl(w,f,"3. Event Memory System",CX,CY,17,P::txtHdr);
    // Stats
    float sx=CX,sy=CY+26.f;
    auto sc=[&](const char* l,int n,sf::Color col){
        fillRect(w,sx,sy,108.f,40.f,P::panel,P::bdr,1.f);
        lbl(w,f,l,sx+3.f,sy+2.f,8,P::txtDim);
        char buf[16];snprintf(buf,16,"%d nodes",n);
        lbl(w,f,buf,sx+3.f,sy+16.f,12,col); sx+=114.f;
    };
    sc("L1 Raw",L1_rawStream.size,P::txt);
    sc("L2 Verified",L2_verifiedStream.size,P::accent);
    sc("L3 Anomaly",L3_anomalyStream.size,L3_anomalyStream.size>0?P::fire:P::txt);
    sc("L4 FwdChain",L4_forwardCorrect.size,P::txtHdr);
    sc("L5 BwdChain",L5_backwardCorrect.size,P::txtHdr);
    sc("L6 Sync",L6_syncChain.size,P::accent);
    sc("L7 LocalLoop",L7_localLoop.size,P::txtDim);
    sc("L9 Emergency",L9_emergencyLoop.size,L9_emergencyLoop.size>0?P::fire:P::txtDim);

    // L1 node visualizer
    float lvy=sy+48.f;
    panelBox(w,f,CX,lvy,CW,52.f,"L1 RAW STREAM  (latest 15 nodes, → direction of time)");
    float nx=CX+4.f,ny=lvy+24.f;
    EventNode* cur=L1_rawStream.head; int skip=L1_rawStream.size-15;
    for(int i=0;i<skip&&cur;i++) cur=cur->next;
    int drawn=0;
    while(cur&&drawn<15){
        fillRect(w,nx,ny,66.f,22.f,cur->isAnomalous?sf::Color{44,7,4}:P::panel,
                 cur->isAnomalous?P::fire:P::bdr,1.f);
        char buf[20];snprintf(buf,20,"Z%d %.0f",cur->zone,cur->value);
        lbl(w,f,buf,nx+2.f,ny+5.f,10,cur->isAnomalous?P::fireBrt:P::txt);
        if(cur->next) fillRect(w,nx+66.f,ny+10.f,7.f,2.f,P::bdr);
        nx+=75.f; cur=cur->next; drawn++;
    }
    if(L1_rawStream.size==0) lbl(w,f,"(empty)",CX+6.f,ny+4.f,11,P::txtDim);

    // L3 anomaly visualizer
    float lvy2=lvy+58.f;
    panelBox(w,f,CX,lvy2,CW,52.f,"L3 ANOMALY STREAM");
    nx=CX+4.f; ny=lvy2+24.f;
    cur=L3_anomalyStream.head; skip=L3_anomalyStream.size-15;
    for(int i=0;i<skip&&cur;i++) cur=cur->next;
    drawn=0;
    while(cur&&drawn<15){
        fillRect(w,nx,ny,66.f,22.f,{44,7,4},P::fire,1.5f);
        char buf[20];snprintf(buf,20,"Z%d %.0f",cur->zone,cur->value);
        lbl(w,f,buf,nx+2.f,ny+5.f,10,P::fireBrt);
        if(cur->next) fillRect(w,nx+66.f,ny+10.f,7.f,2.f,P::fire);
        nx+=75.f; cur=cur->next; drawn++;
    }
    if(L3_anomalyStream.size==0) lbl(w,f,"(no anomalies)",CX+6.f,ny+4.f,11,P::accent);

    g_btns.clear();
    float btn_y=lvy2+60.f;

     vector<SubItem> items={
        {"3.1  Store Event (manual input)",[]{
            showDialog("3.1  Store Event",{
                {"Zone (0-9):","","0"},{"Value:","","25"},
                {"Type  1=TEMP 2=SMOKE 3=HUMIDITY:","","1"}
            },[]( vector< string> v){
                runFeeding(v[0]+"\n"+v[1]+"\n"+v[2]+"\n",[]{
                    int zone,typeChoice; float val;
                     cin>>zone>>val>>typeChoice;
                    const char* types[]={"TEMP","SMOKE","HUMIDITY"};
                    const char* t2=(typeChoice>=1&&typeChoice<=3)?types[typeChoice-1]:"TEMP";
                    float base=(typeChoice==1)?BASELINE_TEMP[zone]:
                               (typeChoice==2)?BASELINE_SMOKE[zone]:BASELINE_HUMIDITY[zone];
                    ingestEvent(zone,val,t2,base);
                     cout<<"Event stored and routed through L1-L10.\n";
                });
            });
        }},
        {"3.2  Traverse Forward (L1-L6)",[]{
            runCap([]{
                L1_rawStream.traverseForward("L1 Raw");
                L2_verifiedStream.traverseForward("L2 Verified");
                L3_anomalyStream.traverseForward("L3 Anomaly");
                L4_forwardCorrect.traverseForward("L4 FwdCorrection");
                L6_syncChain.traverseForward("L6 Sync");
            });
        }},
        {"3.3  Traverse Backward (L4-L5)",[]{
            runCap([]{
                L4_forwardCorrect.traverseBackward("L4 BackwardScan");
                L5_backwardCorrect.traverseBackward("L5 BackwardCorrect");
                 cout<<"[CORRECT] Attempting to fix last anomaly in L5...\n";
                float corrVal=dynamicGrid.interpolateTemp(2,2);
                bool fixed=L5_backwardCorrect.correctLastAnomaly(corrVal);
                if(!fixed) cout<<"  No anomaly found to correct.\n";
            });
        }},
        {"3.4  Circular Event Monitoring",[]{
            runCap([]{
                L7_localLoop.traverseOneCycle("L7 Local Monitor");
                L8_systemLoop.traverseOneCycle("L8 System Monitor");
                L9_emergencyLoop.traverseOneCycle("L9 Emergency Monitor");
                L10_stabilityLoop.traverseOneCycle("L10 Stability Monitor");
            });
        }},
        {"3.5  Restore Last Stable State",[]{
            runCap([]{
                if(rollbackStack.isEmpty()){ cout<<"No saved state to restore.\n";return;}
                bool ok=rollbackStack.pop(dynamicGrid);
                if(ok){ cout<<"[RESTORED] Dynamic grid rolled back to last stable state.\n";
                     cout<<"[INFO] "<<L3_anomalyStream.size<<" anomaly events still in L3.\n";}
            });
        }},
    };

    const float BW=284.f,BH=38.f;
    float bx=CX;
    for(int i=0;i<(int)items.size();i++){
        Btn b; b.r={{bx,btn_y},{BW,BH}}; b.lbl2=items[i].label;
        auto fn=items[i].cb; b.cb=fn; g_btns.push_back(b);
        bx+=BW+5.f;
        if(bx+BW>(float)(WW-6.f)){bx=CX;btn_y+=BH+5.f;}
    }
    Btn back; back.r={{CX,(float)LOG_Y-46.f},{120.f,36.f}};
    back.lbl2="← Back"; back.cb=[]{ g_scr=Scr::MAIN; g_btns.clear(); };
    g_btns.push_back(back);
    for(auto& b:g_btns) b.draw(w,f);
}


//  MENU 4 — Fire Detection and Control

static void renderM4(sf::RenderWindow& w,const sf::Font& f,float t){
    lbl(w,f,"4. Fire Detection and Control",CX,CY,17,P::fire);

    // Risk score mini-bars
    float rx=CX,ry=CY+28.f;
    panelBox(w,f,CX,ry,CW,58.f,"Zone Risk Scores  (Decision Score = 0.4*FireFlag + 0.3*Smoke/100 + 0.3*Temp/100)");
    rx=CX+4.f; float ry2=ry+24.f;
    for(int z=0;z<MAX_ZONES;z++){
        float risk=computeRiskScore(z);
        sf::Color rc=risk>0.6f?P::fire:risk>0.3f?P::warn:P::accent;
        char lbuf[6];snprintf(lbuf,6,"Z%d",z);
        lbl(w,f,lbuf,rx,ry2,9,P::txtDim);
        barFill(w,rx,ry2+12.f,78.f,10.f,risk,{22,22,22},rc);
        char rv[8];snprintf(rv,8,"%.2f",risk);
        lbl(w,f,rv,rx+80.f,ry2+12.f,8,rc);
        rx+=92.f;
    }

    g_btns.clear();
    float btn_y=ry+64.f;

     vector<SubItem> items={
        {"4.1  Detect Fire Risk (Threshold)",[]{
            runCap([]{
                const float w1=0.4f,w2=0.3f,w3=0.3f,THR=0.6f;
                 cout<<"[FIRE RISK] Decision Score per Zone:\n";
                 cout<<"  Zone | Temp  | Smoke | Score | Status\n";
                 cout<<"  -----|-------|-------|-------|--------\n";
                bool any=false;
                for(int z=0;z<MAX_ZONES;z++){int i=sensorData.latestIdx(z);if(i<0)continue;any=true;
                    float t2=sensorData.temperature[z][i],s=sensorData.smoke[z][i];
                    float fireFlag=(t2>TEMP_THRESHOLD||s>SMOKE_THRESHOLD)?1.f:0.f;
                    float score=w1*fireFlag+w2*(s/100.f)+w3*(t2/100.f);
                    bool risk=score>=THR;
                     cout<<"   "<<z<<"   | "<<t2<<" | "<<s<<" | "<<score
                             <<" | "<<(risk?"** FIRE RISK **":"OK")<<"\n";
                    if(risk)Q3_emergency.insert(1,z,score,"FireRisk-Auto");
                }
                if(!any) cout<<"  (no data — add readings via Menu 1 first)\n";
            });
        }},
        {"4.2  Trigger Emergency Alert",[]{
            showDialog("4.2  Trigger Emergency Alert",{
                {"Zone:","","3"},{"Temperature:","","75"},{"Smoke level:","","85"}
            },[]( vector< string> v){
                runFeeding(v[0]+"\n"+v[1]+"\n"+v[2]+"\n",[]{
                    int zone; float temp,smoke;
                     cin>>zone>>temp>>smoke;
                    rollbackStack.push(dynamicGrid,"pre-emergency");
                    int row=zone/GRID_COLS,col=zone%GRID_COLS;
                    dynamicGrid.setCell(row,col,temp,smoke,dynamicGrid.humidity[row][col]);
                    Q3_emergency.insert(1,zone,temp,"EmergencyAlert-Manual");
                    ingestEvent(zone,temp,"TEMP",BASELINE_TEMP[zone]);
                    ingestEvent(zone,smoke,"SMOKE",BASELINE_SMOKE[zone]);
                     cout<<"*** EMERGENCY ALERT TRIGGERED - Zone "<<zone<<" ***\n";
                     cout<<"Grid snapshot saved. Event routed through L1/L3/L9.\n";
                });
            });
        }},
        {"4.3  Priority-Based Fire Response",[]{
            runCap([]{
                 cout<<"[Q3] Processing Emergency Response Queue:\n";
                if(Q3_emergency.isEmpty()){ cout<<"  No emergency tasks pending.\n";return;}
                Task t2; int processed=0;
                while(!Q3_emergency.isEmpty()){Q3_emergency.extractMin(t2);
                     cout<<"  Zone "<<t2.zone<<" score="<<t2.value<<" desc="<<t2.description<<"\n";
                    processed++;}
                 cout<<"  "<<processed<<" tasks processed.\n";
            });
        }},
        {"4.4  Simulate Fire Spread (BFS)",[]{
            showDialog("4.4  Fire Spread BFS",{{"Fire origin zone (0-9):","","3"}},
            []( vector< string> v){
                runFeeding(v[0]+"\n",[]{
                    int startZone;  cin>>startZone;
                    int si=sensorData.latestIdx(startZone);
                    if(si>=0){
                        float t2=sensorData.temperature[startZone][si];
                        float s=sensorData.smoke[startZone][si];
                        float fl=0.f;
                        if(t2>TEMP_THRESHOLD||s>SMOKE_THRESHOLD)
                            fl=0.4f*(t2>TEMP_THRESHOLD?1.f:0.f)+0.3f*(s/100.f)+0.3f*(t2/100.f);
                         cout<<"Fire level at Zone "<<startZone<<" = "<<fl<<"\n";
                        graphList.updateFireCost(startZone,fl);
                    }
                     cout<<"[BFS] Fire spread prediction from Zone "<<startZone<<":\n";
                    bfs(graphList,startZone);
                     cout<<"Each zone above is reachable from fire origin.\n";
                    sysMonitor.record(5,3.5f,10);
                });
            });
        }},
        {"4.5  Allocate Firefighting Resources",[]{
            runCap([]{
                 cout<<"[RESOURCE ALLOCATION] Zone-by-zone:\n";
                 cout<<"  Zone | Risk  | Priority | Water       | Allocation\n";
                const float TW=1000.f,IMPACT=0.8f; float wR=TW;
                for(int z=0;z<MAX_ZONES;z++){float risk=computeRiskScore(z);if(risk==0.f)continue;
                    float pri=risk*IMPACT,wN=risk*200.f,wG=(wR>=wN)?wN:wR;
                    float wRat=(wN>0)?wG/wN:0.f; wR-=wG;
                    const char* wS=wRat>=0.8f?"Sufficient":wRat>=0.4f?"Limited":"Critical";
                    const char* alloc=pri>0.7f?"AllTrucks+Crew+Air":pri>0.4f?"2Trucks+HalfCrew":"Monitor-Only";
                     cout<<"   "<<z<<" | "<<risk<<" | "<<pri<<" | "<<wG<<"L("<<wS<<") | "<<alloc<<"\n";
                    if(pri>0.4f)Q4_multiDecision.enqueue((int)(pri*5),z,pri,"ResourceAlloc");
                }
                 cout<<"\nWater remaining: "<<wR<<"L\n";
                T4_water.display(); T5_fireControl.display(); T6_equipment.display();
                 cout<<"Tasks dispatched to Q4: "<<Q4_multiDecision.count<<"\n";
            });
        }},
    };

    const float BW=252.f,BH=38.f;
    float bx=CX;
    for(int i=0;i<(int)items.size();i++){
        Btn b; b.r={{bx,btn_y},{BW,BH}}; b.lbl2=items[i].label;
        auto fn=items[i].cb; b.cb=fn; g_btns.push_back(b);
        bx+=BW+5.f;
        if(bx+BW>(float)(WW-6.f)){bx=CX;btn_y+=BH+5.f;}
    }
    Btn back; back.r={{CX,(float)LOG_Y-46.f},{120.f,36.f}};
    back.lbl2="← Back"; back.cb=[]{ g_scr=Scr::MAIN; g_btns.clear(); };
    g_btns.push_back(back);
    for(auto& b:g_btns) b.draw(w,f);
}


//  MENU 5 — Task Scheduling

static void renderM5(sf::RenderWindow& w,const sf::Font& f){
    lbl(w,f,"5. Task Scheduling System",CX,CY,17,P::txtHdr);

    // Queue status bars
    float qy=CY+28.f;
    panelBox(w,f,CX,qy,CW,72.f,"Queue Status");
    float qw=(CW-16.f)/4.f;
    auto drawQ=[&](float x,float ht_y,float wd,const char* nm,int cnt,int cap,sf::Color col,bool paused){
        fillRect(w,x,ht_y,wd,50.f,P::panel,P::bdr,1.f);
        lbl(w,f,nm,x+4.f,ht_y+2.f,9,P::txtHdr);
        barFill(w,x+4.f,ht_y+18.f,wd-8.f,10.f,(float)cnt/cap,{22,22,22},paused?P::txtDim:col);
        char buf[30];snprintf(buf,30,"%d/%d%s",cnt,cap,paused?" [PAUSED]":"");
        lbl(w,f,buf,x+4.f,ht_y+32.f,9,paused?P::txtDim:col);
    };
    drawQ(CX+4.f,      qy+16.f,qw,"Q1 Routine",Q1_routine.count,QUEUE_CAPACITY,P::accent,q1Paused);
    drawQ(CX+4.f+qw+4.f, qy+16.f,qw,"Q2 Surveillance",Q2_surveillance.count,QUEUE_CAPACITY,P::txtHdr,q2Paused);
    drawQ(CX+4.f+2*(qw+4.f),qy+16.f,qw,"Q3 Emergency",Q3_emergency.size,QUEUE_CAPACITY,P::fire,false);
    drawQ(CX+4.f+3*(qw+4.f),qy+16.f,qw,"Q4 MultiDecision",Q4_multiDecision.count,QUEUE_CAPACITY,P::warn,false);

    g_btns.clear();
    float btn_y=qy+78.f;

     vector<SubItem> items={
        {"5.1  Add Routine Task (Q1)",[]{
            showDialog("5.1  Add Routine Task (Q1)",{{"Zone:","","0"},{"Sensor value:","","25"}},
            []( vector< string> v){
                runFeeding(v[0]+"\n"+v[1]+"\n",[]{
                    int zone; float val;  cin>>zone>>val;
                    if(q1Paused) cout<<"[Q1 PAUSED] Task queued but not processed.\n";
                    Q1_routine.enqueue(5,zone,val,"RoutineMonitor");
                });
            });
        }},
        {"5.2  Add Surveillance Task (Q2)",[]{
            showDialog("5.2  Add Surveillance Task (Q2)",{{"Zone:","","1"},{"Sensor value:","","30"}},
            []( vector< string> v){
                runFeeding(v[0]+"\n"+v[1]+"\n",[]{
                    int zone; float val;  cin>>zone>>val;
                    if(q2Paused) cout<<"[Q2 PAUSED] Task queued but not processed.\n";
                    Q2_surveillance.enqueue(3,zone,val,"SurveillanceScan");
                });
            });
        }},
        {"5.3  Add Emergency Task (Q3)",[]{
            showDialog("5.3  Add Emergency Task (Q3 Priority)",{
                {"Zone:","","3"},{"Priority (1=highest, 5=lowest):","","1"},{"Sensor value:","","75"}
            },[]( vector< string> v){
                runFeeding(v[0]+"\n"+v[1]+"\n"+v[2]+"\n",[]{
                    int zone,priority; float val;
                     cin>>zone>>priority>>val;
                    Q3_emergency.insert(priority,zone,val,"EmergencyTask");
                });
            });
        }},
        {"5.4  Process Tasks (choose queue)",[]{
            showDialog("5.4  Process Tasks",{{"Queue  1=Q1  2=Q2  3=Q3  4=Q4:","","3"}},
            []( vector< string> v){
                runFeeding(v[0]+"\n",[]{
                    int qc;  cin>>qc; Task t2;
                    if(qc==1){if(q1Paused) cout<<"Q1 is paused.\n";else Q1_routine.dequeue(t2);}
                    else if(qc==2){if(q2Paused) cout<<"Q2 is paused.\n";else Q2_surveillance.dequeue(t2);}
                    else if(qc==3) Q3_emergency.extractMin(t2);
                    else if(qc==4) Q4_multiDecision.dequeue(t2);
                    Q1_routine.display();Q2_surveillance.display();
                    Q3_emergency.display();Q4_multiDecision.display();
                });
            });
        }},
        {"5.5  Pause / Resume Queues",[]{
            showDialog("5.5  Pause / Resume",{{"1=Pause Q1  2=Resume Q1  3=Pause Q2  4=Resume Q2:","","1"}},
            []( vector< string> v){
                runFeeding(v[0]+"\n",[]{
                    int qc;  cin>>qc;
                    if(qc==1){q1Paused=true;  cout<<"Q1 PAUSED.\n";}
                    else if(qc==2){q1Paused=false; cout<<"Q1 RESUMED.\n";}
                    else if(qc==3){q2Paused=true;  cout<<"Q2 PAUSED.\n";}
                    else if(qc==4){q2Paused=false; cout<<"Q2 RESUMED.\n";}
                });
            });
        }},
    };

    const float BW=252.f,BH=38.f;
    float bx=CX;
    for(int i=0;i<(int)items.size();i++){
        Btn b; b.r={{bx,btn_y},{BW,BH}}; b.lbl2=items[i].label;
        auto fn=items[i].cb; b.cb=fn; g_btns.push_back(b);
        bx+=BW+5.f;
        if(bx+BW>(float)(WW-6.f)){bx=CX;btn_y+=BH+5.f;}
    }
    Btn back; back.r={{CX,(float)LOG_Y-46.f},{120.f,36.f}};
    back.lbl2="← Back"; back.cb=[]{ g_scr=Scr::MAIN; g_btns.clear(); };
    g_btns.push_back(back);
    for(auto& b:g_btns) b.draw(w,f);
}


//  MENU 6 — Decision System

static void renderM6(sf::RenderWindow& w,const sf::Font& f){
     vector<SubItem> items={
        {"6.1  Compute Risk Score (all zones)",[]{
            runCap([]{
                 cout<<"[RISK SCORES] w1=0.4(fire) w2=0.3(smoke) w3=0.3(temp)\n";
                 cout<<"  Zone | Score  | Level\n  -----|--------|------\n";
                for(int z=0;z<MAX_ZONES;z++){float sc=computeRiskScore(z);
                    const char* lv=sc>0.7f?"CRITICAL":sc>0.4f?"HIGH":sc>0.f?"LOW":"(no data)";
                     cout<<"   "<<z<<"   |  "<<sc<<"  | "<<lv<<"\n";}
            });
        }},
        {"6.2  Zone-Level Decision Tree (T10)",[]{
            showDialog("6.2  Local Decision (T10)",{{"Zone to evaluate (0-9):","","0"}},
            []( vector< string> v){
                runFeeding(v[0]+"\n",[]{
                    int zone;  cin>>zone;
                    float score=computeRiskScore(zone);
                     cout<<"Zone "<<zone<<" risk score = "<<score<<"\n";
                    T10_localDecision.nodes[1].score=score;
                    T10_localDecision.nodes[2].score=1.f-score;
                    T10_localDecision.evaluate(0.6f);
                    T10_localDecision.display();
                    int best=T10_localDecision.highestTriggeredLeaf();
                    if(best>=0) cout<<"Decision: "<<T10_localDecision.nodes[best].label<<"\n";
                    else        cout<<"Decision: ContinueMonitor (score below threshold)\n";
                });
            });
        }},
        {"6.3  Regional Decision (T11)",[]{
            runCap([]{
                float spr=0.f;
                for(int z=0;z<MAX_ZONES;z++)spr+=computeRiskScore(z);
                spr/=MAX_ZONES;
                 cout<<"Average regional spread rate = "<<spr<<"\n";
                T11_regional.nodes[1].score=spr;
                T11_regional.nodes[2].score=1.f-spr;
                T11_regional.evaluate(0.5f);
                T11_regional.display();
            });
        }},
        {"6.4  Global Emergency Decision (T12)",[]{
            runCap([]{
                float tot=0.f;
                for(int z=0;z<MAX_ZONES;z++)tot+=computeRiskScore(z);
                 cout<<"Total system risk = "<<tot<<" (threshold=3.0)\n";
                if(tot>3.f){T12_global.nodes[1].score=0.9f;T12_global.evaluate(0.8f);
                     cout<<"*** GLOBAL ALERT ACTIVATED ***\n";}
                else{T12_global.nodes[2].score=0.6f;T12_global.evaluate(0.8f);
                     cout<<"System in standby mode.\n";}
                T12_global.display();
            });
        }},
        {"6.5  Execute Final Action",[]{
            runCap([]{
                int wz=0; float ws=0.f;
                for(int z=0;z<MAX_ZONES;z++){float sc=computeRiskScore(z);if(sc>ws){ws=sc;wz=z;}}
                 cout<<"Highest risk: Zone "<<wz<<" score="<<ws<<"\n";
                if(ws>0.6f){Q3_emergency.insert(1,wz,ws,"FinalAction-Emergency");
                     cout<<"Emergency task dispatched to Q3.\n";
                    graphList.updateFireCost(wz,ws);}
                else{Q1_routine.enqueue(5,wz,ws,"FinalAction-Routine");
                     cout<<"Routine task dispatched to Q1.\n";}
            });
        }},
        {"6.6  View Trees (choose category)",[]{
            showDialog("6.6  View Trees",{
                {"1=Structural  2=Terrain  3=Resources\n4=Incidents  5=Decisions  6=All:","","6"}
            },[]( vector< string> v){
                runFeeding(v[0]+"\n",[]{
                    int tc;  cin>>tc;
                    if(tc==1||tc==6){T1_zoneHierarchy.display();T2_subZone.display();}
                    if(tc==2||tc==6) T3_terrain.display();
                    if(tc==3||tc==6){T4_water.display();T5_fireControl.display();T6_equipment.display();}
                    if(tc==4||tc==6){T7_fireClass.display();T8_wildlife.display();T9_human.display();}
                    if(tc==5||tc==6){T10_localDecision.display();T11_regional.display();T12_global.display();}
                });
            });
        }},
    };
    buildSubItems("6. Decision System",items,w,f);
    for(auto& b:g_btns) b.draw(w,f);
}


//  MENU 7 — Spatial Routing

static void renderM7(sf::RenderWindow& w,const sf::Font& f,float t){
    lbl(w,f,"7. Spatial Routing System",CX,CY,17,P::txtHdr);

    // Graph visual
    panelBox(w,f,CX,CY+26.f,440.f,220.f,"G1  Adjacency List Graph");
    float gcx=CX+220.f,gcy=CY+26.f+115.f,gr=88.f;
    sf::Vector2f zp[MAX_GRAPH_NODES];
    for(int i=0;i<MAX_GRAPH_NODES;i++){
        float a=(float)i/MAX_GRAPH_NODES*2.f*3.14159f-1.57f;
        zp[i]={gcx+gr* cos(a),gcy+gr* sin(a)};
    }
    for(int u=0;u<MAX_GRAPH_NODES;u++) for(int j=0;j<graphList.degree[u];j++){
        int v=graphList.edges[u][j].neighbour; if(v<=u)continue;
        bool blk=graphList.edges[u][j].blocked;
        float fw=graphList.edges[u][j].weight;
        sf::Color ec=blk?P::danger:fw>10.f?P::fire:fw>5.f?P::warn:P::bdr;
        sf::Vertex ln[2]={{zp[u],ec},{zp[v],ec}};
        w.draw(ln,2,sf::PrimitiveType::Lines);
        char wb[8];snprintf(wb,8,"%.0f",fw);
        lbl(w,f,wb,(zp[u].x+zp[v].x)/2.f,(zp[u].y+zp[v].y)/2.f,8,ec);
    }
    for(int i=0;i<MAX_GRAPH_NODES;i++){
        float risk=computeRiskScore(i); bool fire=(risk>0.5f);
        sf::CircleShape c(13.f);c.setOrigin({13.f,13.f});c.setPosition(zp[i]);
        c.setFillColor(fire?sf::Color{(uint8_t)(148+48* sin(t*3+i)),34,4}:P::gridOk);
        c.setOutlineColor(fire?P::fire:P::bdr);c.setOutlineThickness(fire?2.f:1.f);w.draw(c);
        char zl[5];snprintf(zl,5,"Z%d",i);
        sf::Text zt(f,zl,9);zt.setFillColor(P::txt);
        auto tb=zt.getLocalBounds();
        zt.setPosition({zp[i].x-tb.size.x/2.f-tb.position.x,zp[i].y-tb.size.y/2.f-tb.position.y});
        w.draw(zt);
    }

    g_btns.clear();
    float btn_y=CY+254.f;

     vector<SubItem> items={
        {"7.1  Load / Display Adj List",[]{
            runCap([]{graphList.display();});
        }},
        {"7.2  Load / Display Adj Matrix",[]{
            runCap([]{graphMatrix.display();});
        }},
        {"7.3  BFS Traversal (Fire Spread)",[]{
            showDialog("7.3  BFS Traversal",{{"Start zone (0-9):","","0"}},
            []( vector< string> v){
                runFeeding(v[0]+"\n",[]{int s; cin>>s;bfs(graphList,s);sysMonitor.record(5,3.5f,8);});
            });
        }},
        {"7.4  DFS Traversal (Deep Analysis)",[]{
            showDialog("7.4  DFS Traversal",{{"Start zone (0-9):","","0"}},
            []( vector< string> v){
                runFeeding(v[0]+"\n",[]{int s; cin>>s;dfs(graphList,s);sysMonitor.record(5,2.8f,6);});
            });
        }},
        {"7.5  Compute Safe Path",[]{
            showDialog("7.5  Safe Path",{{"Source zone:","","0"},{"Destination zone:","","9"}},
            []( vector< string> v){
                runFeeding(v[0]+"\n"+v[1]+"\n",[]{int s,d; cin>>s>>d;safePath(graphList,s,d);});
            });
        }},
        {"7.6  Update Blocked Routes",[]{
            showDialog("7.6  Update Fire Cost",{{"Zone with fire:","","3"},{"Fire level (0.0-1.0):","","0.7"}},
            []( vector< string> v){
                runFeeding(v[0]+"\n"+v[1]+"\n",[]{
                    int zone;float fl; cin>>zone>>fl;
                    graphList.updateFireCost(zone,fl);graphList.display();
                });
            });
        }},
    };

    const float BW=232.f,BH=38.f;
    float bx=CX;
    for(int i=0;i<(int)items.size();i++){
        Btn b; b.r={{bx,btn_y},{BW,BH}}; b.lbl2=items[i].label;
        auto fn=items[i].cb; b.cb=fn; g_btns.push_back(b);
        bx+=BW+5.f;
        if(bx+BW>(float)(WW-6.f)){bx=CX;btn_y+=BH+5.f;}
    }
    Btn back; back.r={{CX,(float)LOG_Y-46.f},{120.f,36.f}};
    back.lbl2="← Back"; back.cb=[]{ g_scr=Scr::MAIN; g_btns.clear(); };
    g_btns.push_back(back);
    for(auto& b:g_btns) b.draw(w,f);
}


//  MENU 8 — Hash Access

static void renderM8(sf::RenderWindow& w,const sf::Font& f){
    lbl(w,f,"8. Hash-Based Fast Access System",CX,CY,17,P::txtHdr);

    // H1 visual
    panelBox(w,f,CX,CY+26.f,CW,72.f,"H1 Primary Index Table  (index = key mod 13)");
    float cw2=(CW-10.f)/HASH_TABLE_SIZE;
    float cy2=CY+48.f;
    for(int i=0;i<HASH_TABLE_SIZE;i++){
        bool occ=H1_primary.table[i].occupied;
        fillRect(w,CX+5.f+i*cw2,cy2,cw2-1.f,36.f,occ?sf::Color{33,52,16}:sf::Color{16,26,18},
                 occ?P::accent:P::bdr,0.5f);
        if(occ){char kb[8];snprintf(kb,8,"k=%d",H1_primary.table[i].key);
            lbl(w,f,kb,CX+6.f+i*cw2,cy2+1.f,8,P::txt);
            char vb[8];snprintf(vb,8,"%.0f",H1_primary.table[i].temperature);
            lbl(w,f,vb,CX+6.f+i*cw2,cy2+14.f,8,P::accent);}
        char ib[5];snprintf(ib,5,"[%d]",i);
        lbl(w,f,ib,CX+5.f+i*cw2,cy2+38.f,8,P::txtDim);
    }

    g_btns.clear();
    float btn_y=CY+104.f;

     vector<SubItem> items={
        {"8.1  Insert Data (H1)",[]{
            showDialog("8.1  Insert into H1",{
                {"Zone key (0-99):","","3"},{"Temperature:","","30"},
                {"Smoke:","","10"},{"Humidity:","","55"}
            },[]( vector< string> v){
                runFeeding(v[0]+"\n"+v[1]+"\n"+v[2]+"\n"+v[3]+"\n",[]{
                    int key;float temp,smoke,humidity;
                     cin>>key>>temp>>smoke>>humidity;
                    H1_primary.insert(key,temp,smoke,humidity);
                    H2_collision.insert(key,temp,smoke,humidity);
                    sysMonitor.record(6,0.5f,H1_primary.count);
                });
            });
        }},
        {"8.2  Retrieve Data (O(1))",[]{
            showDialog("8.2  Retrieve by Key",{{"Zone key to retrieve:","","3"}},
            []( vector< string> v){
                runFeeding(v[0]+"\n",[]{
                    int key; cin>>key;
                    HashRecord rec;
                    bool found=H3_cache.get(key,rec);
                    if(!found){found=H1_primary.lookup(key,rec);
                        if(found){ cout<<"[H1] Found: T="<<rec.temperature
                                           <<" S="<<rec.smoke<<" H="<<rec.humidity<<"\n";
                            H3_cache.put(rec);}
                        else  cout<<"[H1] Key "<<key<<" not found.\n";}
                    else  cout<<"[CACHE HIT] T="<<rec.temperature<<" S="<<rec.smoke<<" H="<<rec.humidity<<"\n";
                });
            });
        }},
        {"8.3  Handle Collisions (H2)",[]{
            runCap([]{
                 cout<<"[H2] Collision demo: keys 3 and 16 both hash to "<<(3%HASH_TABLE_SIZE)<<"\n";
                H2_collision.insert(3,28.f,10.f,55.f);
                H2_collision.insert(16,32.f,20.f,50.f);
                H2_collision.display();
            });
        }},
        {"8.4  Update Cache (H3)",[]{
            runCap([]{
                 cout<<"[H3] Caching latest sensor readings...\n";
                for(int z=0;z<MAX_ZONES;z++){int i=sensorData.latestIdx(z);if(i<0)continue;
                    HashRecord rec; rec.key=z;
                    rec.temperature=sensorData.temperature[z][i];
                    rec.smoke=sensorData.smoke[z][i];
                    rec.humidity=sensorData.humidity[z][i];
                    rec.occupied=true; H3_cache.put(rec);}
            });
        }},
        {"8.5  View Index Tables",[]{
            runCap([]{H1_primary.display();H2_collision.display();H3_cache.display();});
        }},
    };

    const float BW=252.f,BH=38.f;
    float bx=CX;
    for(int i=0;i<(int)items.size();i++){
        Btn b; b.r={{bx,btn_y},{BW,BH}}; b.lbl2=items[i].label;
        auto fn=items[i].cb; b.cb=fn; g_btns.push_back(b);
        bx+=BW+5.f;
        if(bx+BW>(float)(WW-6.f)){bx=CX;btn_y+=BH+5.f;}
    }
    Btn back; back.r={{CX,(float)LOG_Y-46.f},{120.f,36.f}};
    back.lbl2="← Back"; back.cb=[]{ g_scr=Scr::MAIN; g_btns.clear(); };
    g_btns.push_back(back);
    for(auto& b:g_btns) b.draw(w,f);
}


//  MENU 9 — System Monitoring

static void renderM9(sf::RenderWindow& w,const sf::Font& f){
    lbl(w,f,"9. System Monitoring",CX,CY,17,P::txtHdr);

    // Module gauges
    float gx=CX,gy=CY+28.f;
    for(int i=0;i<sysMonitor.count;i++){
        float load=(sysMonitor.modules[i].capacity>0)?
                   (float)sysMonitor.modules[i].activeTasks/sysMonitor.modules[i].capacity:0.f;
        sf::Color gc=sysMonitor.modules[i].bottleneck?P::danger:load>0.4f?P::warn:P::accent;
        fillRect(w,gx,gy,148.f,74.f,P::panel,gc,1.f);
        lbl(w,f,sysMonitor.modules[i].name,gx+3.f,gy+3.f,9,P::txtHdr);
        barFill(w,gx+3.f,gy+18.f,142.f,9.f,load,{22,22,22},gc);
        char buf[32];snprintf(buf,32,"%.0f%% / %.1fms",load*100.f,sysMonitor.modules[i].latencyMs);
        lbl(w,f,buf,gx+3.f,gy+31.f,9,P::txtDim);
        char buf2[20];snprintf(buf2,20,"Tasks: %d/%d",sysMonitor.modules[i].activeTasks,sysMonitor.modules[i].capacity);
        lbl(w,f,buf2,gx+3.f,gy+46.f,9,P::txtDim);
        lbl(w,f,sysMonitor.modules[i].bottleneck?"BOTTLENECK":"OK",gx+3.f,gy+59.f,9,gc);
        gx+=154.f;
        if(gx+148.f>(float)(WW-4.f)){gx=CX;gy+=80.f;}
    }

    g_btns.clear();
    float btn_y=gy+82.f; if(btn_y<CY+164.f)btn_y=CY+164.f;

     vector<SubItem> items={
        {"9.1  Monitor System Load",[]{
            runCap([]{
                 cout<<"[MONITOR] Simulating module loads...\n";
                sysMonitor.record(0,1.2f,45);sysMonitor.record(1,2.1f,60);
                sysMonitor.record(2,0.8f,20);sysMonitor.record(3,3.5f,40);
                sysMonitor.record(4,4.2f,35);sysMonitor.record(5,9.1f,38);
                sysMonitor.record(6,0.6f,25);sysMonitor.record(7,1.8f,55);
                sysMonitor.displayHealth();
            });
        }},
        {"9.2  Track Execution Time",[]{
            runCap([]{
                 cout<<"[LATENCY] Running BFS from Zone 0...\n";
                int before=globalTick;
                bfs(graphList,0);
                int after=globalTick;
                float simLat=(float)(after-before)*0.3f+1.5f;
                 cout<<"Simulated BFS latency = "<<simLat<<" ms\n";
                sysMonitor.record(5,simLat,10);
            });
        }},
        {"9.3  Detect Bottlenecks",[]{
            runCap([]{
                sysMonitor.displayHealth();
                int bn=sysMonitor.detectBottleneck();
                 cout<<">>> Primary bottleneck: "<<sysMonitor.modules[bn].name<<" <<<\n";
                 cout<<"Recommendation: reduce task load or increase capacity.\n";
            });
        }},
        {"9.4  Optimize Performance",[]{
            runCap([]{sysMonitor.optimize();sysMonitor.displayHealth();});
        }},
        {"9.5  View System Health",[]{
            runCap([]{
                sysMonitor.displayHealth();
                 cout<<"Q1 pending: "<<Q1_routine.count<<"\n";
                 cout<<"Q2 pending: "<<Q2_surveillance.count<<"\n";
                 cout<<"Q3 pending: "<<Q3_emergency.size<<"\n";
                 cout<<"L1 raw:     "<<L1_rawStream.size<<"\n";
                 cout<<"L2 verified:"<<L2_verifiedStream.size<<"\n";
                 cout<<"L3 anomaly: "<<L3_anomalyStream.size<<"\n";
                 cout<<"Stack depth:"<<rollbackStack.depth()<<"\n";
            });
        }},
    };

    const float BW=252.f,BH=38.f;
    float bx=CX;
    for(int i=0;i<(int)items.size();i++){
        Btn b; b.r={{bx,btn_y},{BW,BH}}; b.lbl2=items[i].label;
        auto fn=items[i].cb; b.cb=fn; g_btns.push_back(b);
        bx+=BW+5.f;
        if(bx+BW>(float)(WW-6.f)){bx=CX;btn_y+=BH+5.f;}
    }
    Btn back; back.r={{CX,(float)LOG_Y-46.f},{120.f,36.f}};
    back.lbl2="← Back"; back.cb=[]{ g_scr=Scr::MAIN; g_btns.clear(); };
    g_btns.push_back(back);
    for(auto& b:g_btns) b.draw(w,f);
}


//  MENU 10 — Scenarios

static void renderM10(sf::RenderWindow& w,const sf::Font& f){
    lbl(w,f,"10. Scenario Simulation",CX,CY,17,P::txtHdr);

    struct SI{const char* num,*name,*desc; sf::Color col;  function<void()> fn;};
    SI scenes[]={
        {"10.1","Cascading Fire & Resource Conflict",
         "Zone 3 ignites → spreads Z4/Z6. Priority queuing, rollback, resource allocation.",
         P::fire, []{runCap([]{scenario1();});}},
        {"10.2","Sensor Failure & Reconstruction",
         "Zone 2 sensors fail. Invalid filtered. Spatial interpolation rebuilds data.",
         P::warn, []{runCap([]{scenario2();});}},
        {"10.3","Multi-Factor Anomaly Escalation",
         "Wildlife, fire-risk, human events simultaneously. Combined BFS escalation.",
         P::fireBrt,[]{runCap([]{scenario3();});}},
        {"10.4","System Overload & Load Redistribution",
         "Mass updates overload queues. Priority rebalancing + cache acceleration.",
         P::danger,[]{runCap([]{scenario4();});}},
        {"10.5","Global Multi-Zone Emergency Sync",
         "Multi-zone emergency, async inconsistencies. Global state reconstruction.",
         P::smokeC,[]{runCap([]{scenario5();});}},
    };

    g_btns.clear();
    float sy=CY+28.f;
    for(auto& sc:scenes){
        fillRect(w,CX,sy,CW,62.f,P::panel,sc.col,1.f);
        lbl(w,f,sc.num,CX+7.f,sy+8.f,13,sc.col);
        lbl(w,f,sc.name,CX+62.f,sy+8.f,13,P::txt);
        lbl(w,f,sc.desc,CX+62.f,sy+26.f,11,P::txtDim);
        Btn rb; rb.r={{CX+CW-166.f,sy+13.f},{152.f,34.f}};
        rb.lbl2= string("Run ")+sc.num;
        auto fn=sc.fn; rb.cb=fn; g_btns.push_back(rb);
        sy+=68.f;
    }

    // Run All
    Btn all; all.r={{CX,sy},{292.f,40.f}};
    all.lbl2="10.6  Run ALL 5 Scenarios (Full Demo)";
    all.cb=[]{runCap([]{
        scenario1();scenario2();scenario3();scenario4();scenario5();
         cout<<"\n===== FULL SYSTEM SIMULATION COMPLETE =====\n";
    });};
    g_btns.push_back(all);

    Btn back; back.r={{CX,(float)LOG_Y-46.f},{120.f,36.f}};
    back.lbl2="← Back"; back.cb=[]{ g_scr=Scr::MAIN; g_btns.clear(); };
    g_btns.push_back(back);
    for(auto& b:g_btns) b.draw(w,f);
}


//  MAIN

int main(){
    // Init all data structures
    initPool();
    sensorData.init(); staticGrid.initBaseline(); dynamicGrid.initBaseline();
    rollbackStack.init();
    L1_rawStream.init(); L2_verifiedStream.init(); L3_anomalyStream.init();
    L4_forwardCorrect.init(); L5_backwardCorrect.init(); L6_syncChain.init();
    L7_localLoop.init(CIRC_LOOP_SIZE); L8_systemLoop.init(CIRC_LOOP_SIZE);
    L9_emergencyLoop.init(CIRC_LOOP_SIZE); L10_stabilityLoop.init(CIRC_LOOP_SIZE);
    Q1_routine.init("Q1-Routine"); Q2_surveillance.init("Q2-Surveillance");
    Q3_emergency.init(); Q4_multiDecision.init("Q4-MultiDecision");
    H1_primary.init(); H2_collision.init(); H3_cache.init();
    sysMonitor.init(); loadDefaultGraph(); buildAllTrees();

    g_log.push_back("IFAMDS GUI ready — all data structures initialised.");
    g_log.push_back("Use sidebar to navigate menus. Each button replicates the original console menu exactly.");
    g_log.push_back("Buttons needing input open a dialog. Tab = next field, Enter = confirm, Esc = cancel.");

    // ── SFML Window 
    sf::RenderWindow win(
        sf::VideoMode({(unsigned)WW,(unsigned)WH}),
        "IFAMDS — Forest Advisory & Multi-Structure Decision System",
        sf::Style::Titlebar|sf::Style::Close);
    win.setFramerateLimit(60);

    // Load font — MSYS2 common paths
    sf::Font font;
    const char* fonts[]={
        "C:/msys64/mingw64/share/fonts/TTF/DejaVuSansMono.ttf",
        "C:/Windows/Fonts/consola.ttf",
        "C:/Windows/Fonts/cour.ttf",
        "C:/Windows/Fonts/lucon.ttf",
        "/usr/share/fonts/truetype/dejavu/DejaVuSansMono.ttf",
        "/usr/share/fonts/truetype/ubuntu/UbuntuMono-R.ttf",
        nullptr
    };
    for(int i=0;fonts[i];i++) if(font.openFromFile(fonts[i])) break;

    sf::Clock clk;
    float totalT=0.f;
    srand((unsigned)time(nullptr));

    while(win.isOpen()){
        float dt=clk.restart().asSeconds(); totalT+=dt;

        while(auto ev=win.pollEvent()){
            if(ev->is<sf::Event::Closed>()) win.close();

            // ── SPLASH: any key/click advances to MAIN ──
            if(g_scr == Scr::SPLASH) {
                if(auto* ke=ev->getIf<sf::Event::KeyPressed>()) {
                    if(ke->code==sf::Keyboard::Key::Enter ||
                       ke->code==sf::Keyboard::Key::Space)
                        g_scr=Scr::MAIN;
                }
                if(ev->is<sf::Event::MouseButtonPressed>())
                    g_scr=Scr::MAIN;
                continue;  // skip all other event handling while on splash
            }

            if(auto* ke=ev->getIf<sf::Event::KeyPressed>())
                dlgKeyPressed(ke->code);

            if(auto* te=ev->getIf<sf::Event::TextEntered>())
                dlgTextEntered(te->unicode);

            if(auto* mb=ev->getIf<sf::Event::MouseButtonPressed>()){
                if(mb->button==sf::Mouse::Button::Left&&!g_dlg.active){
                    sf::Vector2i mp(mb->position.x,mb->position.y);
                    for(auto& b:g_navBtns) if(b.hit(mp)&&b.cb){b.cb();break;}
                    for(auto& b:g_btns)    if(b.hit(mp)&&b.cb){b.cb();break;}
                }
            }
            if(auto* mm=ev->getIf<sf::Event::MouseMoved>()){
                sf::Vector2i mp(mm->position.x,mm->position.y);
                for(auto& b:g_navBtns) b.hov=b.hit(mp);
                for(auto& b:g_btns)    b.hov=b.hit(mp);
            }
            if(auto* mw=ev->getIf<sf::Event::MouseWheelScrolled>())
                if(mw->wheel==sf::Mouse::Wheel::Vertical)
                    g_logScroll= max(0.f,g_logScroll+(mw->delta<0?-2.f:2.f));
        }

        tickSparks(dt);
        win.clear(P::bg);

        // Splash screen — fullscreen, no HUD overlay
        if(g_scr == Scr::SPLASH){
            drawSplash(win, font, totalT);
            drawSparks(win);
            win.display();
            continue;
        }

        // Content
        switch(g_scr){
            case Scr::MAIN: drawMainMenu(win,font,totalT); break;
            case Scr::M1:   renderM1(win,font);             break;
            case Scr::M2:   renderM2(win,font,totalT);      break;
            case Scr::M3:   renderM3(win,font);             break;
            case Scr::M4:   renderM4(win,font,totalT);      break;
            case Scr::M5:   renderM5(win,font);             break;
            case Scr::M6:   renderM6(win,font);             break;
            case Scr::M7:   renderM7(win,font,totalT);      break;
            case Scr::M8:   renderM8(win,font);             break;
            case Scr::M9:   renderM9(win,font);             break;
            case Scr::M10:  renderM10(win,font);            break;
            default: break;
        }

        drawLogPanel(win,font);
        drawSparks(win);
        drawSidebar(win,font);
        drawTopBar(win,font);
        drawDialog(win,font);    // modal, always on top

        win.display();
    }
    return 0;
}