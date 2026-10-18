from PIL import Image, ImageDraw
import random
W,H=480,320; HUD=26; TW,TH=32,16
def q(c): # quantize to RGB565
    r,g,b=c; return ((r>>3)<<3,(g>>2)<<2,(b>>3)<<3)
FONT={'A':"01110100011000111111100011000110001",'B':"11110100011111010001100011000111110",'C':"01111100001000010000100001000001111",
'D':"11110100011000110001100011000111110",'E':"11111100001111010000100001000011111",'F':"11111100001111010000100001000010000",
'G':"01111100001001110001100011000101111",'H':"10001100011111110001100011000110001",'I':"11111001000010000100001000010011111",
'K':"10001100101110010010100011000110001",'L':"10000100001000010000100001000011111",'M':"10001110111010110001100011000110001",
'N':"10001110011010110011100011000110001",'O':"01110100011000110001100011000101110",'P':"11110100011111010000100001000010000",
'R':"11110100011111010100100101000110001",'S':"01111100000111000001000011000111110",'T':"11111001000010000100001000010000100",
'U':"10001100011000110001100011000101110",'V':"10001100011000110001010100101000100",'W':"10001100011000110101101011101110001",
'Y':"10001010100010000100001000010000100",'X':"10001010100010001010100011000110001",'0':"01110100111010111001100011000101110",
'1':"00100011000010000100001000010001110",'2':"01110100010001000100010001000011111",'3':"11110000010111000001000011000111110",
'4':"10010100101001011111000100001000010",'5':"11111100001111000001000011000111110",'6':"01110100001111010001100011000101110",
'7':"11111000010001000100010000100001000",'8':"01110100010111010001100011000101110",'9':"01110100011000101111000010000101110",
' ':"0"*35,':':"00000001000000000000001000000000000",'-':"00000000000111000000000000000000000",'!':"00100001000010000100000000000000100",
',':"00000000000000000000001000010001000",'.':"00000000000000000000000000000000100",'>':"10000010000010000010001000100010000",'<':"00001000100010001000001000001000001",'x':"00000000001000101010001000101010001"}
def text(d,x,y,s,c,sc=1,sh=None):
    for ch in s:
        g=FONT.get(ch,FONT[' '])
        for i,v in enumerate(g):
            if v=='1':
                px,py=x+(i%5)*sc,y+(i//5)*sc
                if sh: d.rectangle([px+sc//2,py+sc//2,px+sc+sc//2-1,py+sc+sc//2-1],fill=sh)
                d.rectangle([px,py,px+sc-1,py+sc-1],fill=c)
        x+=6*sc
def tw(s,sc=1): return len(s)*6*sc
class Scene:
    def __init__(s,ox,oy): s.ox,s.oy=ox,oy
    def P(s,a,b,z=0): return (s.ox+(a+b)*TW/2, s.oy+(b-a)*TH/2 - z)
def tile(d,S,a,b,c,edge=None):
    pts=[S.P(a,b),S.P(a+1,b),S.P(a+1,b+1),S.P(a,b+1)]
    d.polygon(pts,fill=c,outline=edge)
def shade(c,f): return q(tuple(max(0,min(255,int(v*f))) for v in c))
def box(d,S,a,b,L,Wd,h,top,z0=0,outline=(0,0,0)):
    A=lambda x,y,z:S.P(x,y,z0+z)
    # side facing +b
    d.polygon([A(a,b+Wd,0),A(a+L,b+Wd,0),A(a+L,b+Wd,h),A(a,b+Wd,h)],fill=shade(top,.62),outline=outline)
    d.polygon([A(a,b,0),A(a,b+Wd,0),A(a,b+Wd,h),A(a,b,h)],fill=shade(top,.8),outline=outline)
    d.polygon([A(a,b,h),A(a+L,b,h),A(a+L,b+Wd,h),A(a,b+Wd,h)],fill=top,outline=outline)
def car(d,S,a,b,body,win=(40,60,90),z=0,L=1.6,Wd=.8,lights=None):
    d.polygon([S.P(a+.1,b+.1),S.P(a+L+.1,b+.1),S.P(a+L+.1,b+Wd+.1),S.P(a+.1,b+Wd+.1)],fill=(0,0,0)) if z==0 else None
    box(d,S,a,b,L,Wd,6,body,z)
    box(d,S,a+.35,b+.1,L*.5,Wd-.2,5,win,z+6)
    box(d,S,a+.45,b+.15,L*.3,Wd-.3,1,body,z+11,outline=None)
    if lights:
        x,y=S.P(a+L,b+.2,z+4); d.rectangle([x-1,y-1,x+1,y+1],fill=lights)
        x,y=S.P(a+L,b+Wd-.2,z+4); d.rectangle([x-1,y-1,x+1,y+1],fill=lights)
def hud(d,score,lives,weap,bg,fg,acc):
    d.rectangle([0,H-HUD,W,H],fill=bg); d.line([0,H-HUD,W,H-HUD],fill=acc,width=2)
    text(d,6,H-19,"SCORE",acc,1); text(d,40,H-21,"%06d"%score,fg,2)
    text(d,140,H-19,"LIVES",acc,1)
    for i in range(lives):
        x=176+i*16; d.rectangle([x,H-17,x+11,H-9],fill=(240,240,240)); d.rectangle([x+3,H-20,x+8,H-17],fill=(80,120,200))
    text(d,240,H-19,"WPN",acc,1); text(d,262,H-21,weap,fg,2)
    text(d,380,H-19,"TIME",acc,1)
    d.rectangle([408,H-19,472,H-9],outline=fg); d.rectangle([410,H-17,450,H-11],fill=acc)
def bands(j,spec):
    for lo,hi,c in spec:
        if lo<=j<hi: return c
    return spec[-1][2]
def ground(d,S,f):
    for a in range(-30,30):
        for b in range(-30,30):
            c=f(a,b)
            if c: tile(d,S,a,b,*c) if isinstance(c[0],tuple) else tile(d,S,a,b,c)
def save(img,n):
    img=img.convert('RGB').point(lambda v:v)  # then quantize
    px=img.load()
    for y in range(H):
        for x in range(W): px[x,y]=q(px[x,y])
    img.save(f"mockups/{n}.png"); img.resize((W*2,H*2),Image.NEAREST).save(f"mockups/{n}_2x.png")
random.seed(3)
# road occupies b in [ROAD0,ROAD1)
def scene(theme):
    img=Image.new('RGB',(W,H)); d=ImageDraw.Draw(img); S=Scene(160,150)
    T=theme
    def f(a,b):
        if T['name']=='bridge' and (b<-4 or b>=5): 
            return (T['water2'] if (a*3+b*5)%7==0 else T['water'])
        if -4<=b<-3 or 4<=b<5: return T['shoulder']
        if -3<=b<4:
            return T['road2'] if (a+b)%2 else T['road']
        return T['grass2'] if (a*7+b*3)%5==0 else T['grass']
    ground(d,S,f)
    # lane dashes
    for a in range(-30,30,2):
        for lb in (-0.55,1.95):
            p=[S.P(a,lb+.45),S.P(a+1,lb+.45),S.P(a+1,lb+.55),S.P(a,lb+.55)]
            d.polygon(p,fill=T['dash'])
    # edge lines
    for lb in (-3.1,3.95):
        d.polygon([S.P(-30,lb),S.P(30,lb),S.P(30,lb+.15),S.P(-30,lb+.15)],fill=T['edge'])
    return img,d,S
def objs_sort(lst): return sorted(lst,key=lambda o:(o[2]-o[1]))
# ---- 1 day
day=dict(name='day',grass=(56,148,56),grass2=(40,120,48),road=(96,96,104),road2=(104,104,112),shoulder=(200,184,120),dash=(248,248,248),edge=(248,220,64))
img,d,S=scene(day)
ob=[]
for a in range(-14,14):
    for b in list(range(-12,-5))+list(range(6,14)):
        if random.random()<.12: ob.append(('tree',a,b))
ob+= [('car',-0.2,0.6,(232,232,240),'P'),('car',3.5,-2.2,(200,40,40),'E'),('car',6,2.4,(240,200,40),'C'),
      ('car',-3.5,2.6,(40,120,220),'C'),('car',9,-0.6,(30,30,34),'E'),('van',11.5,-2.3)]
d.polygon([S.P(2.2,1.5),S.P(3.2,1.3),S.P(3.6,2.2),S.P(2.6,2.6)],fill=(20,20,28))  # oil
d.polygon([S.P(2.5,1.7),S.P(2.9,1.6),S.P(3.0,1.9)],fill=(70,50,110))
for o in objs_sort(ob):
    if o[0]=='tree':
        _,a,b=o; box(d,S,a+.4,b+.4,.2,.2,6,(110,70,30)); box(d,S,a+.1,b+.1,.8,.8,12,(30,110,40),6)
    elif o[0]=='car': car(d,S,o[1],o[2],o[3],lights=(255,255,160))
    else: box(d,S,o[1],o[2],2.6,1.1,16,(60,60,150)); text(d,*map(int,S.P(o[1]+.4,o[2]+.1,10)),"W",(255,255,255))
# bullets
for k in range(3):
    x,y=S.P(1.8+k*.7,1.0,4); d.rectangle([x-1,y-1,x+1,y+1],fill=(255,240,80))
hud(d,12450,3,"MG",(16,16,24),(248,248,248),(248,200,40))
save(img,"01_day_highway")
# ---- 2 night neon
night=dict(name='night',grass=(16,16,40),grass2=(24,16,56),road=(28,28,44),road2=(32,32,52),shoulder=(248,0,200),dash=(0,248,248),edge=(248,0,200))
img,d,S=scene(night)
ob=[]
for a in range(-14,14,3):
    ob.append(('lamp',a,-4.6)); ob.append(('lamp',a+1,5.4))
for a in range(-14,14):
    for b in list(range(-12,-6))+list(range(7,14)):
        if random.random()<.08: ob.append(('bld',a,b,random.choice([20,28,36,44])))
ob+= [('car',-0.2,0.6,(0,220,248),'P'),('car',3.5,-2.2,(248,40,120),'E'),('car',6.2,2.4,(160,80,248),'C'),('car',9,-0.6,(248,40,120),'E')]
for o in objs_sort(ob):
    if o[0]=='lamp':
        _,a,b=o; box(d,S,a,b,.15,.15,22,(120,120,140)); x,y=S.P(a,b,24); d.ellipse([x-3,y-2,x+3,y+2],fill=(255,240,160))
        gx,gy=S.P(a,b+ (1 if b<0 else -1)); 
    elif o[0]=='bld':
        _,a,b,h=o; box(d,S,a,b,.9,.9,h,(40,30,80))
        for z in range(6,h-2,6):
            x,y=S.P(a+.3,b+.9,z); 
            if random.random()<.6: d.rectangle([x,y-2,x+2,y],fill=random.choice([(248,220,80),(0,248,248)]))
    else: car(d,S,o[1],o[2],o[3],lights=(255,255,255))
# headlight beam
x0,y0=S.P(1.4,1.0,2); d.polygon([(x0,y0),S.P(4,0.2),S.P(4,2.2)],outline=(200,200,120))
x,y=S.P(1.8,1.0,4); d.line([x,y,x+50,y-25],fill=(0,248,248),width=2)
hud(d,38020,2,"MISSILE",(8,0,24),(0,248,248),(248,0,200))
save(img,"02_night_neon")
# ---- 3 bridge
br=dict(name='bridge',grass=(56,148,56),grass2=(40,120,48),road=(128,120,112),road2=(136,128,120),shoulder=(168,160,150),dash=(248,248,248),edge=(248,248,248),water=(32,96,200),water2=(96,168,240))
img,d,S=scene(br)
ob=[]
for a in range(-14,14,2): ob.append(('rail',a,-4.2)); ob.append(('rail',a,4.8))
ob+=[('boat',2,-8),('boat',-4,8.5),('boatE',7,7),('car',-0.2,0.6,(232,232,240),'P'),('car',4.5,1.8,(200,40,40),'E'),('car',8,-1.8,(240,140,40),'C')]
d.polygon([S.P(2.6,-1.6),S.P(3.6,-1.6),S.P(3.6,-0.6),S.P(2.6,-0.6)],fill=(200,40,0)); text(d,*map(int,S.P(2.9,-1.0)),"!",(255,255,0))
for o in objs_sort(ob):
    if o[0]=='rail': box(d,S,o[1],o[2],1.8,.12,4,(220,220,220)); box(d,S,o[1],o[2],.15,.15,7,(160,160,160))
    elif o[0].startswith('boat'):
        _,a,b=o; c=(240,240,240) if o[0]=='boat' else (60,60,60)
        for k in range(4): d.line([S.P(a-.5-k*.6,b+.2+k*.25),S.P(a-.5-k*.6,b+.6-k*.25)],fill=(220,240,255))
        box(d,S,a,b,2,.8,5,c); box(d,S,a+.4,b+.15,.8,.5,6,(60,90,140) if o[0]=='boat' else (200,30,30),5)
    else: car(d,S,o[1],o[2],o[3],lights=(255,255,160))
hud(d,24880,3,"SMOKE",(16,16,24),(248,248,248),(80,200,248))
save(img,"03_bridge_river")
# ---- 4 title
img,d,S=scene(night); 
d.rectangle([0,0,W,H],fill=None)
ov=Image.new('RGB',(W,H),(0,0,0)); img=Image.blend(img,ov,.55); d=ImageDraw.Draw(img); S=Scene(160,150)
for a,b,c in [(-2,2.5,(0,220,248)),(1,-0.5,(248,40,120))]: car(d,S,a,b,c,lights=(255,255,255))
t="AGENT RUN"; text(d,(W-tw(t,6))//2,30,t,(248,220,40),6,sh=(200,0,120))
s="ROAD SPY 2026"; text(d,(W-tw(s,2))//2,84,s,(0,248,248),2)
for i,(l,c) in enumerate([("TURN KNOB TO STEER",(248,248,248)),("PRESS TO FIRE",(248,248,248)),("> PRESS TO START <",(248,0,200))]):
    text(d,(W-tw(l,2))//2,210+i*22,l,c,2)
text(d,(W-tw("HI 038020"))//2,300,"HI 038020",(160,160,200))
save(img,"04_title")
