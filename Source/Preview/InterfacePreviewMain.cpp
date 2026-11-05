#include <juce_gui_extra/juce_gui_extra.h>
#include <array>
#include <vector>
#include <cmath>

namespace
{
const juce::Colour bg {0x0F,0x11,0x1A}, panel {0x14,0x17,0x22}, fill {0x17,0x1A,0x26};
const juce::Colour line {0x42,0x49,0x5C}, ink {0xEE,0xF0,0xF7}, muted {0xAAB2C4};
const juce::Colour blue {0x8D,0xB4,0xFF}, selected {0x25,0x34,0x5E};
const juce::Colour knobTrack {0x3A,0x3E,0x4C}, knobValue {0xE8,0xEA,0xF0};
const juce::Colour knobName {0xD8,0xDB,0xE6}, valueReadout {0x8B,0x90,0xA3};
const juce::Colour sliderTrack {0x2C,0x2F,0x3C}, sliderFill {0xC9,0xCC,0xD8};
constexpr float pi=3.14159265358979323846f;
struct Sector { const char* name; int ring; float from,to; const char* label; };
struct StandardSector { const char* name; float inner, outer, from, to; bool gapStart, gapEnd; const char* line1; const char* line2; };
const std::array<StandardSector,19> standard {{
 {"Piano",62,100,180,0,false,false,"PIANO",""},
 {"Violins I",108,253,180,142,false,true,"VIOLINS I",""},
 {"Violins II",110,192,142,90,true,false,"VIOLINS II",""},
 {"Violas",110,192,90,40,false,true,"VIOLAS",""},
 {"Celli",108,217,40,0,true,false,"CELLI",""},
 {"Basses",228,313,40,0,true,false,"BASSES",""},
 {"Piccolos",202,254,142,121,true,false,"PICCOLOS",""},
 {"Flutes",202,254,121,100,false,false,"FLUTES",""},
 {"Oboes",202,254,100,79,false,false,"OBOES",""},
 {"Clarinets",202,254,79,58,false,false,"CLARINETS",""},
 {"Bassoons",202,254,58,40,false,true,"BASSOONS",""},
 {"Horns",262,318,180,143,false,true,"HORNS",""},
 {"Harp / Celeste",262,318,143,111,true,false,"HARP /","CELESTE"},
 {"Percussion",262,318,111,69,false,false,"PERCUSSION",""},
 {"Tuned Percussion",262,318,69,40,false,true,"TUNED","PERCUSSION"},
 {"Trumpets",326,388,40,30,false,false,"TRUMPETS",""},
 {"Tenor Trombones",326,388,30,20,false,false,"TENOR","TROMBONES"},
 {"Bass Trombones",326,388,20,12,false,false,"BASS","TROMBONES"},
 {"Tuba",326,388,12,0,false,false,"TUBA",""}
}};
const std::array<Sector,18> antiphonal {{
 {"Piano",0,180,0,"PIANO"},{"Violins I",1,180,140,"VIOLINS I"},
 {"Celli",1,140,90,"CELLI"},{"Violas",1,90,40,"VIOLAS"},
 {"Violins II",1,40,0,"VIOLINS II"},{"Basses",2,150,118,"BASSES"},
 {"Piccolos / Flutes",2,118,94,"PICCOLOS /\nFLUTES"},{"Oboes",2,94,70,"OBOES"},
 {"Clarinets",2,70,46,"CLARINETS"},{"Bassoons",2,46,22,"BASSOONS"},
 {"Harp / Celeste",3,180,152,"HARP /\nCELESTE"},{"Horns",3,148,120,"HORNS"},
 {"Trumpets",3,120,98,"TRUMPETS"},{"Tenor Trombones",3,98,78,"TENOR\nTROMBONES"},
 {"Bass Trombones",3,78,60,"BASS\nTROMBONES"},{"Tuba",3,60,44,"TUBA"},
 {"Percussion",4,125,68,"PERCUSSION"},{"Tuned Percussion",4,68,38,"TUNED PERCUSSION"}
}};
const std::array<float,5> inner {{62,108,213,273,333}}, outer {{100,205,265,325,385}};
}

class Preview final : public juce::Component
{
public:
 Preview(){ setSize(1000,700); setWantsKeyboardFocus(true); juce::Logger::writeToLog("Standard layout sector count: "+juce::String(standard.size())); }
 void paint(juce::Graphics& g) override
 {
  g.fillAll(bg); auto area=getLocalBounds().toFloat();
  g.setColour(ink); g.setFont(juce::Font(16,juce::Font::bold)); g.drawText("ORCHESTRAL STUDIO REVERB",24,12,330,26,juce::Justification::centredLeft);
  g.setColour(muted); g.setFont(9); g.drawText("CONCERT SPACES FOR ORCHESTRAL MUSIC",26,37,350,15,juce::Justification::centredLeft);
  drawText(g,"PRESET",390,11,65,14,9,muted);
  rounded(g,{450,10,280,34},line,fill); drawText(g,presets[preset],462,10,245,34,13,ink);
  rounded(g,{738,10,58,34},line,panel); drawText(g,"PREV",738,10,58,34,9,ink,juce::Justification::centred);
  rounded(g,{800,10,58,34},line,panel); drawText(g,"NEXT",800,10,58,34,9,ink,juce::Justification::centred);
  rounded(g,{866,10,58,34},line,panel); drawText(g,"SAVE",866,10,58,34,9,ink,juce::Justification::centred);
  rounded(g,{930,10,42,34},line,panel); drawText(g,"MENU",930,10,42,34,8,ink,juce::Justification::centred);
  auto left=juce::Rectangle<float>(14,58,258,398), stage=juce::Rectangle<float>(284,58,702,398);
  rounded(g,left,line,panel); rounded(g,stage,line,panel);
  drawText(g,"ROOM & REVERB",28,70,210,20,10,ink);
  g.setColour(line); g.drawLine(28,98,258,98);
  drawText(g,"ROOM",29,108,200,18,10,ink);
  knob(g,90,166,"ROOM SIZE",juce::String("1.20 ")+juce::String::charToString(0x00D7)); knob(g,203,166,"PRE-DELAY","54 ms");
  g.setColour(line); g.drawLine(28,226,258,226);
  drawText(g,"REVERB TIME",29,236,200,18,10,ink);
  knob(g,145,305,"DECAY","2.80 s");
  drawText(g,layout==0?"STANDARD ORCHESTRA LAYOUT":"CLASSICAL / EUROPEAN (ANTIPHONAL) LAYOUT",302,69,265,18,9,ink);
  rounded(g,{574,64,112,28},line,layout==0?selected:bg);
  drawText(g,"STANDARD",574,64,112,28,9,ink,juce::Justification::centred);
  rounded(g,{692,64,164,28},line,layout==1?selected:bg);
  drawText(g,"CLASSICAL / EUROPEAN",692,64,164,28,9,ink,juce::Justification::centred);
  drawText(g,juce::String("SELECTED: ")+selectedName.toUpperCase(),860,69,110,18,8,blue,juce::Justification::centredRight);
  drawStage(g,stage.withTrimmedLeft(14).withTrimmedRight(14).withTrimmedTop(39).withTrimmedBottom(12));
  auto bottom=juce::Rectangle<float>(14,468,972,218); rounded(g,bottom,line,panel);
  drawSpacedHeading(g,"DECAY",34,480,110,16,10.0f,knobName);
  knob(g,92,538,"LOW DECAY","2.80 s",0.78f); knob(g,195,538,"HIGH DECAY","4.20 s",0.87f);
  knob(g,298,538,"CROSSOVER","1.20 kHz",0.55f); knob(g,401,538,"DAMPING","35 %",0.72f);
  g.setColour(line); g.drawLine(465,482,465,592);
  drawSpacedHeading(g,"MODULATION",482,480,140,16,10.0f,knobName);
  knob(g,550,538,"DEPTH","12 %",0.42f); knob(g,652,538,"RATE","0.30 Hz",0.68f);
  g.setColour(line); g.drawLine(715,482,715,592);
  drawSpacedHeading(g,"STEREO",734,480,100,16,10.0f,knobName);
  knob(g,800,538,"WIDTH","120 %",0.8f); knob(g,905,538,"DIFFUSION","75 %",0.7f);
  g.setColour(line); g.drawLine(28,600,972,600);
  drawText(g,"DRY / WET",34,609,120,17,10,ink);
  slider(g,84,631,270,"Dry",0.65f,"65 %"); slider(g,84,656,270,"Wet",0.35f,"35 %");
  g.setColour(line); g.drawLine(390,612,390,675);
  drawText(g,"OUTPUT",410,609,100,17,10,ink);
  slider(g,463,644,180,"Gain",0.5f,"0.0 dB");
  g.setColour(line); g.drawLine(690,612,690,675);
  drawText(g,"L",713,625,12,12,9,ink); drawText(g,"R",713,646,12,12,9,ink);
  meter(g,731,628,220,7,0.78f); meter(g,731,649,220,7,0.72f);
  drawText(g,"-60       -48       -36       -24       -12       -6          0",731,660,220,12,8,muted);
 }
 void mouseDown(const juce::MouseEvent& e) override
 {
  auto p=e.position;
  if(p.y>=64&&p.y<=94&&p.x>=574&&p.x<=856)
  {
   layout=p.x<688?0:1;
   selectedName=layout==0?"Violins I":"Violins II";
   hoveredName.clear();repaint();return;
  }
  if(p.y>=10&&p.y<=44&&p.x>=800&&p.x<=858){preset=(preset+1)%presets.size();repaint();return;}
  if(p.y>=10&&p.y<=44&&p.x>=738&&p.x<800){preset=(preset+presets.size()-1)%presets.size();repaint();return;}
  auto r=stageRect(); auto centre=juce::Point<float>(r.getCentreX(),r.getBottom()-1);
  if(!r.contains(p))return;
  if(layout==0){ for(const auto& s:standard) if(standardSectorPath(s,centre,r).contains(p)){selectedName=s.name;repaint();break;} }
  else { for(const auto& s:antiphonal) if(sectorPath(s,centre,r).contains(p)){selectedName=s.name;repaint();break;} }
 }
private:
 std::array<juce::String,6> presets{{"Concert Hall","Small Chamber","Scoring Stage","Cathedral","Studio Room","Symphonic Hall"}};
 int preset=0,layout=0; juce::String selectedName="Violins I", hoveredName;
 juce::Rectangle<float> stageRect()const{return {298,97,674,345};}
 void mouseMove(const juce::MouseEvent& e) override
 {
  juce::String next;
  if(layout==0){auto r=stageRect();auto centre=juce::Point<float>(r.getCentreX(),r.getBottom()-1);for(const auto& s:standard)if(standardSectorPath(s,centre,r).contains(e.position)){next=s.name;break;}}
  if(next!=hoveredName){hoveredName=next;repaint();}
 }
 void mouseExit(const juce::MouseEvent&) override {if(hoveredName.isNotEmpty()){hoveredName.clear();repaint();}}
 static void rounded(juce::Graphics&g,juce::Rectangle<float>r,juce::Colour stroke,juce::Colour fillc){g.setColour(fillc);g.fillRoundedRectangle(r,9);g.setColour(stroke);g.drawRoundedRectangle(r,9,1);}
 static juce::Font uiFont(float size,bool bold=false)
 {
  juce::FontOptions options;
  auto font=juce::Font(options.withName("Inter").withHeight(size).withStyle(bold?"Bold":"Regular"));
  if(!font.getTypefaceName().containsIgnoreCase("Inter"))
   font=juce::Font(options.withName("Arial").withHeight(size).withStyle(bold?"Bold":"Regular"));
  return font;
 }
 static void drawText(juce::Graphics&g,const juce::String&s,int x,int y,int w,int h,float size,juce::Colour c,juce::Justification j=juce::Justification::centredLeft){g.setColour(c);g.setFont(uiFont(size));g.drawFittedText(s,{x,y,w,h},j,2);}
 static void knob(juce::Graphics&g,float cx,float cy,const juce::String&name,const juce::String&value,float amount=0.68f)
 {
  constexpr float radius=19.0f;
  const float start=juce::degreesToRadians(135.0f),end=juce::degreesToRadians(405.0f);
  juce::Path trackArc,valueArc;
  trackArc.addCentredArc(cx,cy,radius,radius,0.0f,start,end,true);
  const float angle=start+(end-start)*juce::jlimit(0.0f,1.0f,amount);
  valueArc.addCentredArc(cx,cy,radius,radius,0.0f,start,angle,true);
  const juce::PathStrokeType stroke(2.2f,juce::PathStrokeType::curved,juce::PathStrokeType::rounded);
  g.setColour(knobTrack);g.strokePath(trackArc,stroke);
  g.setColour(knobValue);g.strokePath(valueArc,stroke);
  const juce::Point<float> pointer(std::sin(angle),-std::cos(angle));
  g.setColour(juce::Colours::white);
  g.drawLine(juce::Line<float>(juce::Point<float>(cx,cy)+pointer*8.0f,juce::Point<float>(cx,cy)+pointer*16.5f),2.0f);
  drawText(g,name,int(cx-48),int(cy+24),96,13,9,knobName,juce::Justification::centred);
  drawText(g,value,int(cx-48),int(cy+38),96,15,10,valueReadout,juce::Justification::centred);
 }
 static void slider(juce::Graphics&g,int x,int y,int w,const juce::String&name,float v,const juce::String&value)
 {
  drawText(g,name,x-48,y-8,42,18,9,knobName,juce::Justification::centredRight);
  g.setColour(sliderTrack);g.fillRoundedRectangle(float(x),float(y),float(w),4,2);
  g.setColour(sliderFill);g.fillRoundedRectangle(float(x),float(y),float(w)*v,4,2);
  g.setColour(juce::Colours::white);g.fillEllipse(float(x)+w*v-5,float(y)-3,10,10);
  drawText(g,value,x+w+9,y-8,55,18,9,valueReadout);
 }
 static void meter(juce::Graphics&g,int x,int y,int w,int h,float value)
 {
  constexpr int segments=48;const float segWidth=float(w)/float(segments);
  for(int i=0;i<segments;++i)
  {
   const bool active=float(i)<float(segments)*value;
   g.setColour(active?sliderFill:sliderTrack.withAlpha(0.9f));
   g.fillRoundedRectangle(float(x)+float(i)*segWidth,float(y),juce::jmax(1.0f,segWidth-1.0f),float(h),1.0f);
  }
 }
 static void drawSpacedHeading(juce::Graphics& g,const juce::String& text,int x,int y,int w,int h,float size,juce::Colour colour)
 {
  juce::Font font=uiFont(size,true);
  float total=0.0f;
  for(int i=0;i<text.length();++i)total+=font.getStringWidth(text.substring(i,i+1))+1.4f;
  float cursor=float(x)+(float(w)-total)*0.5f;
  g.setColour(colour);
  for(int i=0;i<text.length();++i)
  {
   auto ch=text.substring(i,i+1);const float cw=font.getStringWidth(ch);
   g.drawText(ch,juce::Rectangle<float>(cursor,float(y),cw+1.0f,float(h)),juce::Justification::centred,false);
   cursor+=cw+1.4f;
  }
 }
 static float standardScale(juce::Rectangle<float> r)
 { constexpr float maxRadius=388.0f*1.6f; return juce::jmin(r.getWidth()/(2.0f*maxRadius),r.getHeight()/maxRadius); }
 static float juceAngle(float degrees) { return juce::degreesToRadians(90.0f-degrees); }
 static float shiftedEdge(float degrees,float radius,bool isStart,bool gap)
 {
  if(!gap||radius<=0.0f)return degrees;
  const float delta=std::asin(juce::jlimit(0.0f,0.999f,5.0f/radius))*180.0f/pi;
  return degrees+(isStart?-delta:delta);
 }
 static juce::Path standardSectorPath(const StandardSector& s,juce::Point<float> centre,juce::Rectangle<float> bounds)
 {
  const float scale=standardScale(bounds);
  const float rOuter=s.outer*1.6f*scale,rInner=s.inner*1.6f*scale;
  auto point=[&](float radius,float degrees)
  {
   const float angle=juce::degreesToRadians(degrees);
   return juce::Point<float>(centre.x+radius*std::cos(angle),centre.y-radius*std::sin(angle));
  };
  auto gapShift=[](float gap,float radius)
  {
   return gap>0.0f?juce::radiansToDegrees(std::asin(juce::jlimit(0.0f,0.999f,gap/radius))):0.0f;
  };
  const float outerStart=s.from-gapShift(s.gapStart?5.0f:0.0f,rOuter);
  const float outerEnd=s.to+gapShift(s.gapEnd?5.0f:0.0f,rOuter);
  const float innerStart=s.from-gapShift(s.gapStart?5.0f:0.0f,rInner);
  const float innerEnd=s.to+gapShift(s.gapEnd?5.0f:0.0f,rInner);
  juce::Path path;
  auto addArc=[&](float radius,float from,float to,bool first)
  {
   const int steps=juce::jmax(2,int(std::ceil(std::abs(from-to)/1.0f)));
   for(int i=0;i<=steps;++i)
   {
    const float t=float(i)/float(steps);
    const auto p=point(radius,from+(to-from)*t);
    if(first&&i==0)path.startNewSubPath(p);else path.lineTo(p);
   }
  };
  addArc(rOuter,outerStart,outerEnd,true);
  path.lineTo(point(rInner,innerEnd));
  addArc(rInner,innerEnd,innerStart,false);
  path.closeSubPath();
  return path;
 }
 juce::Font standardFont(float size) const
 {
  juce::FontOptions opts; auto f=juce::Font(opts.withName("Inter").withHeight(size));
  if(!f.getTypefaceName().containsIgnoreCase("Inter")) f=juce::Font(opts.withName("Arial").withHeight(size));
  return f;
 }
 void fitStandardLabel(juce::Graphics& g,const StandardSector& s,const juce::Path& path,juce::Point<float> c,juce::Rectangle<float> r)
 {
  const float scale=standardScale(r),mid=(s.from+s.to)*0.5f;
  const float radius=(s.inner+s.outer)*0.5f*1.6f*scale;
  const float a=juce::degreesToRadians(mid);
  const juce::Point<float> anchor(c.x+std::cos(a)*radius,c.y-std::sin(a)*radius);
  const juce::String lines[2]={s.line1,s.line2}; const int count=juce::String(s.line2).isEmpty()?1:2;
  float size=12.0f; juce::Font font=standardFont(size);
  auto widthOf=[&](const juce::String& t,const juce::Font& f){juce::GlyphArrangement ga;ga.addLineOfText(f,t,0.0f,0.0f);return juce::jmax(float(f.getStringWidth(t)),float(ga.getBoundingBox(0,-1,true).getWidth()));};
  const float chordWidth=2.0f*radius*float(std::sin(std::abs(juce::degreesToRadians(s.from-s.to))*0.5f))-8.0f;
   const float localWidth=juce::jmax(12.0f,chordWidth);
  auto maxLineWidth=[&](){float w=0.0f;for(int i=0;i<count;++i)w=juce::jmax(w,float(widthOf(lines[i],font)));return w;};
  while(maxLineWidth()>localWidth&&size>9.0f){size=juce::jmax(9.0f,size-0.5f);font=standardFont(size);}
  float h=font.getHeight()*float(count);
  juce::Rectangle<float> box(anchor.x-localWidth*0.5f,anchor.y-h*0.5f,localWidth,h);
  auto cornersInside=[&](){return path.contains(box.getTopLeft())&&path.contains(box.getTopRight())&&path.contains(box.getBottomLeft())&&path.contains(box.getBottomRight());};
  while(!cornersInside()&&size>9.0f){size=juce::jmax(9.0f,size-0.5f);font=standardFont(size);h=font.getHeight()*float(count);box={anchor.x-localWidth*0.5f,anchor.y-h*0.5f,localWidth,h};}
  if(!cornersInside())juce::Logger::writeToLog("Standard label outside sector: "+juce::String(s.name));
  g.setColour(ink);g.setFont(font);
  for(int i=0;i<count;++i)g.drawText(lines[i],box.getX(),box.getY()+float(i)*font.getHeight(),box.getWidth(),font.getHeight(),juce::Justification::centred,false);
 }
 template<size_t N> void drawStandardSectors(juce::Graphics& g,const std::array<StandardSector,N>& sectors,juce::Point<float> c,juce::Rectangle<float> r)
 {
  std::array<juce::Path,N> paths;
  for(size_t i=0;i<N;++i){paths[i]=standardSectorPath(sectors[i],c,r);g.setColour(fill);g.fillPath(paths[i]);}
  const juce::PathStrokeType outline(2.0f,juce::PathStrokeType::curved,juce::PathStrokeType::rounded);
  for(size_t i=0;i<N;++i){g.setColour(ink);g.strokePath(paths[i],outline);}
  auto state=[&](const juce::String& name,juce::Colour colour,float width){for(size_t i=0;i<N;++i)if(juce::String(sectors[i].name)==name){g.setColour(colour);g.fillPath(paths[i]);g.setColour(blue);g.strokePath(paths[i],juce::PathStrokeType(width,juce::PathStrokeType::curved,juce::PathStrokeType::rounded));}};
  if(hoveredName.isNotEmpty()&&hoveredName!=selectedName)state(hoveredName,juce::Colour(0x22,0x28,0x42),2.0f);
  if(selectedName.isNotEmpty())state(selectedName,selected,2.5f);
  for(size_t i=0;i<N;++i)fitStandardLabel(g,sectors[i],paths[i],c,r);
 }
 static juce::Path sectorPath(const Sector&s,juce::Point<float>c,juce::Rectangle<float>r)
 {
  float scale=juce::jmin(r.getWidth()/1200.0f,r.getHeight()/519.75f);
  float ri=inner[size_t(s.ring)]*1.35f*scale,ro=outer[size_t(s.ring)]*1.35f*scale;
  juce::Path p;for(int i=0;i<=64;++i){float a=juce::degreesToRadians(s.from+(s.to-s.from)*i/64.0f);juce::Point<float>q(c.x+std::cos(a)*ro,c.y-std::sin(a)*ro);if(i==0)p.startNewSubPath(q);else p.lineTo(q);}
  for(int i=64;i>=0;--i){float a=juce::degreesToRadians(s.from+(s.to-s.from)*i/64.0f);p.lineTo(c.x+std::cos(a)*ri,c.y-std::sin(a)*ri);}p.closeSubPath();return p;
 }
 template<size_t N> void drawSectors(juce::Graphics&g,const std::array<Sector,N>&sectors,juce::Point<float>c,juce::Rectangle<float>r)
 {
  for(const auto&s:sectors){auto p=sectorPath(s,c,r);bool sel=juce::String(s.name)==selectedName;g.setColour(sel?selected:fill);g.fillPath(p);g.setColour(sel?blue:ink.withAlpha(0.92f));g.strokePath(p,juce::PathStrokeType(sel?2.5f:1.5f));
   float a=juce::degreesToRadians((s.from+s.to)*0.5f),rad=(inner[size_t(s.ring)]+outer[size_t(s.ring)])*0.675f*scaleFor(r);
   float x=c.x+std::cos(a)*rad,y=c.y-std::sin(a)*rad;float width=juce::jmax(40.0f,rad*2*std::sin(std::abs(juce::degreesToRadians(s.from-s.to))*0.5f)-8);
   const float labelSize=s.ring==0?10.0f:(s.to-s.from<25?9.0f:9.5f);
   drawText(g,s.label,int(x-width*0.5f),int(y-14),int(width),28,labelSize,sel?juce::Colours::white:ink,juce::Justification::centred);
  }
 }
 static float scaleFor(juce::Rectangle<float>r){return juce::jmin(r.getWidth()/1200.0f,r.getHeight()/519.75f);}
 void drawStage(juce::Graphics&g,juce::Rectangle<float>r)
 {
  auto c=juce::Point<float>(r.getCentreX(),r.getBottom()-1);
  if(layout==0)drawStandardSectors(g,standard,c,r);else drawSectors(g,antiphonal,c,r);
 }
};

class PreviewWindow final:public juce::DocumentWindow
{
public:PreviewWindow():DocumentWindow("Orchestral Studio Reverb — Interface Preview",bg,allButtons){setUsingNativeTitleBar(true);setContentOwned(new Preview(),true);setResizable(false,false);centreWithSize(1000,700);setVisible(true);}
 void closeButtonPressed()override{juce::JUCEApplication::getInstance()->systemRequestedQuit();}
};
class App final:public juce::JUCEApplication
{
public:const juce::String getApplicationName()override{return "Orchestral Studio Reverb Preview";}const juce::String getApplicationVersion()override{return "0.2.0";}bool moreThanOneInstanceAllowed()override{return false;}
 void initialise(const juce::String&)override{window=std::make_unique<PreviewWindow>();}void shutdown()override{window.reset();}
private:std::unique_ptr<PreviewWindow>window;
};
START_JUCE_APPLICATION(App)
