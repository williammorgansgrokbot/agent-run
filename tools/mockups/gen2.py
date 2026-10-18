src=open('gen.py').read().split('random.seed(3)')[0]; exec(src)
import random; random.seed(7)
class Sc:
    def __init__(s,ox,oy,k=1.0): s.ox,s.oy,s.k=ox,oy,k
    def P(s,a,b,z=0): return (s.ox+(a+b)*16*s.k, s.oy+(b-a)*8*s.k - z*s.k)
def base(T,S,f):
    img=Image.new('RGB',(W,H),T.get('bg',(0,0,0))); d=ImageDraw.Draw(img)
    for a in range(-40,40):
        for b in range(-40,40):
            c=f(a,b)
            if c: d.polygon([S.P(a,b),S.P(a+1,b),S.P(a+1,b+1),S.P(a,b+1)],fill=c)
    return img,d
def roadf(T,r0=-3,r1=4,extra=None):
    def f(a,b):
        if extra:
            e=extra(a,b)
            if e: return e
        if r0-1<=b<r0 or r1<=b<r1+1: return T['shoulder']
        if r0<=b<r1: return T['road']
        return T['g2'] if (a*7+b*3)%5==0 else T['g']
    return f
def marks(d,S,T,r0=-3,r1=4,amin=-40,amax=40,lanes=(-0.55,1.95)):
    for a in range(amin,amax,2):
        for lb in lanes: d.polygon([S.P(a,lb+.45),S.P(a+1,lb+.45),S.P(a+1,lb+.55),S.P(a,lb+.55)],fill=T['dash'])
    for lb in (r0-.1,r1-.05): d.polygon([S.P(amin,lb),S.P(amax,lb),S.P(amax,lb+.15),S.P(amin,lb+.15)],fill=T['edge'])
def detail(d,S,T,r0=-3,r1=4,n=40,amin=-20,amax=20):  # subtle cracks/patches
    for _ in range(n):
        a=random.uniform(amin,amax); b=random.uniform(r0+.2,r1-.4)
        x,y=S.P(a,b); 
        if random.random()<.5: d.line([x,y,x+4*S.k,y-2*S.k],fill=T['road2'])
        else: d.polygon([S.P(a,b),S.P(a+.4,b),S.P(a+.4,b+.3),S.P(a,b+.3)],fill=T['road2'])
def dsort(l): return sorted(l,key=lambda o:(o[2]-o[1]))
def finish(img,n):
    save(img,n)
# ---------- 5 desert sunset
T=dict(g=(216,140,72),g2=(208,132,68),road=(72,56,64),road2=(64,48,56),shoulder=(168,96,56),dash=(248,216,120),edge=(248,216,120),bg=(248,140,64))
S=Sc(160,150); img,d=base(T,S,roadf(T)); detail(d,S,T); marks(d,S,T)
ob=[('mesa',4,-11,2,2,70),('mesa',-6,-10,2,3,50),('mesa',11,-9,1.5,1.5,84),('mesa',-1,11,2,2,64),('mesa',9,10,2.5,1.5,44),('mesa',-9,10,1.5,2,76)]
for a in range(-14,14):
    for b in list(range(-9,-5))+list(range(6,10)):
        if random.random()<.08: ob.append(('cact',a,b))
ob+=[('car',-0.2,0.6,(232,232,240)),('car',4,-2.2,(40,40,48)),('car',7,2.4,(80,160,200)),('rock',2.5,1.4)]
for o in dsort(ob):
    if o[0]=='mesa':
        _,a,b,L,Wd,h=o
        # long sunset shadow
        d.polygon([S.P(a,b+Wd),S.P(a+L,b+Wd),S.P(a+L-1.5,b+Wd+2.5),S.P(a-1.5,b+Wd+2.5)],fill=(176,104,64))
        box(d,S,a,b,L,Wd,h,(176,80,56))
        for z in range(8,h,10): d.line([S.P(a,b+Wd,z),S.P(a+L,b+Wd,z)],fill=(136,56,48))
    elif o[0]=='cact':
        _,a,b=o; box(d,S,a+.45,b+.45,.15,.15,12,(48,120,64)); box(d,S,a+.25,b+.45,.15,.15,5,(48,120,64),5)
    elif o[0]=='rock':
        _,a,b=o; box(d,S,a,b,.6,.5,5,(120,96,88)); box(d,S,a+.7,b+.3,.4,.4,3,(120,96,88))
    else: car(d,S,o[1],o[2],o[3],lights=(255,240,160))
for k in range(3): x,y=S.P(1.8+k*.7,1.0,4); d.rectangle([x-1,y-1,x+1,y+1],fill=(255,248,120))
hud(d,15760,3,"MG",(40,16,24),(248,224,176),(248,140,64)); finish(img,"05_desert_sunset")
# ---------- 6 snow, BIG scale (k=1.75 -> 56x28 tiles, cars ~56px)
T=dict(g=(232,240,248),g2=(224,234,246),road=(88,92,104),road2=(104,108,120),shoulder=(184,192,208),dash=(248,248,248),edge=(248,200,64))
S=Sc(150,120,1.75)
def ice(a,b): return None
img,d=base(T,S,roadf(T,-2,3))
for p in [(3,0.2,1.6,1.2),(-1,-1.5,1.2,.9)]:
    a,b,L,Wd=p; d.polygon([S.P(a,b),S.P(a+L,b+.2),S.P(a+L,b+Wd),S.P(a+.2,b+Wd+.1)],fill=(168,216,248),outline=(216,240,255))
detail(d,S,T,-2,3,25,-10,10); marks(d,S,T,-2,3,lanes=(0.0,))
ob=[]
for a in range(-10,10):
    for b in list(range(-8,-3))+list(range(4,9)):
        if random.random()<.2: ob.append(('pine',a,b))
ob+=[('car',-0.6,0.9,(232,40,40)),('car',2.8,-1.6,(40,40,48)),('car',5.5,1.2,(64,112,200))]
for o in dsort(ob):
    if o[0]=='pine':
        _,a,b=o; box(d,S,a+.42,b+.42,.16,.16,6,(96,64,40))
        for i,(s,z) in enumerate([(.8,4),(.6,10),(.38,16)]):
            m=(1-s)/2; box(d,S,a+m,b+m,s,s,7,(32,96,72),z); box(d,S,a+m+.05,b+m+.05,s-.1,s-.1,1,(240,248,255),z+7,outline=None)
    else: car(d,S,o[1],o[2],o[3],lights=(255,255,200))
hud(d,9310,3,"OIL",(16,24,40),(248,248,248),(120,200,248)); finish(img,"06_snow_mountain_bigsprites")
# ---------- 7 city downtown with intersection + helicopter
T=dict(g=(72,72,84),g2=(72,72,84),road=(60,60,68),road2=(52,52,60),shoulder=(140,136,128),dash=(248,248,248),edge=(248,220,64))
S=Sc(160,150)
def cross(a,b):
    if 5<=a<10 and not(-3<=b<4):
        if 4.6<=a<5 or 10<=a<10.4: return T['shoulder']
        return T['road']
    if 4<=a<5 and not(-4<=b<5) or 10<=a<11 and not(-4<=b<5): return T['shoulder']
img,d=base(T,S,roadf(T,extra=cross)); detail(d,S,T,n=30)
for seg in [(-40,4),(11,40)]:
    for a in range(seg[0],seg[1],2):
        for lb in (-0.55,1.95): d.polygon([S.P(a,lb+.45),S.P(min(a+1,seg[1]),lb+.45),S.P(min(a+1,seg[1]),lb+.55),S.P(a,lb+.55)],fill=T['dash'])
for lb in (-3.1,3.95):
    for seg in [(-40,4),(11,40)]: d.polygon([S.P(seg[0],lb),S.P(seg[1],lb),S.P(seg[1],lb+.15),S.P(seg[0],lb+.15)],fill=T['edge'])
for i in range(6):  # zebra crossings
    for a0 in (4.2,10.2):
        b=-2.8+i*1.1; d.polygon([S.P(a0,b),S.P(a0+.6,b),S.P(a0+.6,b+.5),S.P(a0,b+.5)],fill=(220,220,220))
ob=[]
for (a,b) in [(-2,-8),(-7,-9),(1,-12),(12,-8),(15,-12),(-4,7),(-9,8),(1,9),(13,6),(16,9),(-13,-7)]:
    ob.append(('sky',a,b,random.choice([40,56,72,90])))
ob+=[('car',-0.2,0.6,(232,232,240)),('car',3.2,-2.2,(200,40,40)),('car',7,6,(240,200,40)),('car',12.5,2.4,(64,140,80)),('car',7.5,-6.5,(64,112,200))]
for o in dsort(ob):
    if o[0]=='sky':
        _,a,b,h=o; box(d,S,a,b,3,3,h,(96,104,128))
        for z in range(8,h-4,8):
            for t in range(3): 
                x,y=S.P(a+.4+t*.9,b+3,z); d.rectangle([x,y-3,x+5,y],fill=(248,232,120) if random.random()<.4 else (56,64,88))
    else: car(d,S,o[1],o[2],o[3],lights=(255,255,180))
# helicopter shadow + bombs
sx,sy=S.P(3.6,1.4); d.ellipse([sx-34,sy-14,sx+34,sy+14],fill=(36,36,44))
hx,hy=S.P(2.2,1.4,70); box(d,S,1.4,0.9,2.0,1.1,14,(56,88,48),70); box(d,S,2.1,1.0,.9,.9,6,(120,180,220),76); box(d,S,3.4,1.3,1.8,.3,5,(56,88,48),76); box(d,S,5.0,1.25,.3,.4,10,(56,88,48),76)
hx,hy=S.P(2.4,1.45,92); d.line([hx-48,hy+10,hx+48,hy-10],fill=(220,220,220),width=2); d.line([hx-40,hy-14,hx+40,hy+14],fill=(170,170,170),width=2); d.rectangle([hx-1,hy-1,hx+1,hy+5],fill=(40,40,40))
for z in (48,26): x,y=S.P(2.0,1.4,z); d.ellipse([x-3,y-3,x+3,y+3],fill=(16,16,16)); d.point((x-1,y-1),fill=(200,200,200))
x,y=S.P(1.2,2.0); d.ellipse([x-10,y-5,x+10,y+5],fill=(248,136,32)); d.ellipse([x-5,y-3,x+5,y+3],fill=(255,240,120))
hud(d,51200,1,"MISSILE",(16,16,24),(248,248,248),(248,64,64)); finish(img,"07_city_heli_attack")
# ---------- 8 coastal dusk boathouse ramp
T=dict(g=(64,104,88),g2=(60,98,84),road=(64,60,80),road2=(56,52,72),shoulder=(184,160,136),dash=(248,232,200),edge=(248,232,200),water=(40,64,128),water2=(232,128,120))
S=Sc(130,170)
def coast(a,b):
    if a>=6: return T['water2'] if (a*7+b*11)%29==0 else T['water']
    if a>=5 and -6<=b<6 and not(-3<=b<4): return (216,192,152)
    if a>=5: return T['water']  # beach
    if b>=7: return T['water2'] if (a*7+b*11)%29==0 else T['water']
    if b>=6: return (216,192,152)
img,d=base(T,S,roadf(T,extra=coast)); detail(d,S,T,amax=4); marks(d,S,T,amax=4)
# ramp: sloped concrete from a=4..7 going into water
d.polygon([S.P(4,-3),S.P(7.5,-3),S.P(7.5,4),S.P(4,4)],fill=(150,144,140)); 
for i in range(8): d.line([S.P(4+i*.45,-3),S.P(4+i*.45,4)],fill=(120,116,116))
d.polygon([S.P(6.6,-3),S.P(7.5,-3),S.P(7.5,4),S.P(6.6,4)],fill=(72,104,160))
d.text
ob=[('bh',5.2,-7.5),('car',-1,0.6,(232,232,240)),('car',2.5,-2.2,(40,40,48)),('pboat',9,0.6),('eboat',13,-2),('palm',1,-6),('palm',-3,-5.5),('palm',-6,6.2)]
for o in dsort(ob):
    if o[0]=='bh':
        _,a,b=o; box(d,S,a,b,3,3,28,(136,80,56)); box(d,S,a-.2,b-.2,3.4,3.4,4,(176,48,48),28)
        x,y=S.P(a+.9,b+3,0); d.rectangle([x,y-20,x+18,y-4],fill=(32,24,24))
        x,y=S.P(a+.6,b+3,24); text(d,int(x),int(y)-4,"BOATS",(248,232,120))
    elif o[0]=='palm':
        _,a,b=o; box(d,S,a+.45,b+.45,.12,.12,22,(120,88,56)); box(d,S,a+.1,b+.1,.8,.8,3,(40,120,72),22)
    elif o[0]=='pboat':
        _,a,b=o
        for k in range(4): d.line([S.P(a-.4-k*.6,b+.1+k*.2),S.P(a-.4-k*.6,b+.7-k*.2)],fill=(220,240,255))
        box(d,S,a,b,2,.9,6,(232,232,240)); box(d,S,a+.5,b+.15,.8,.6,5,(40,60,90),6)
    elif o[0]=='eboat':
        _,a,b=o; box(d,S,a,b,2,.8,5,(60,60,60)); box(d,S,a+.4,b+.15,.8,.5,6,(200,30,30),5)
    else: car(d,S,o[1],o[2],o[3],lights=(255,240,180))
# transform sparkle at ramp
x,y=S.P(6.8,1,6); 
for dx,dy in [(-8,0),(8,0),(0,-6),(0,6),(-5,-4),(5,4)]: d.line([x,y,x+dx,y+dy],fill=(255,248,160))
t="BOAT MODE!"; text(d,(W-tw(t,2))//2,8,t,(248,232,120),2,sh=(120,40,80))
hud(d,30440,2,"TORPEDO",(24,16,40),(248,232,200),(232,128,120)); finish(img,"08_coast_boathouse")
