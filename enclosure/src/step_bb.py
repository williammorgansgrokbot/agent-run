import gmsh,sys
gmsh.initialize(); gmsh.option.setNumber("General.Terminal",0)
gmsh.model.occ.importShapes("E32R40T_3D/E32R40T_3D.step"); gmsh.model.occ.synchronize()
vols=gmsh.model.getEntities(3)
print(len(vols),"volumes")
print("ALL",gmsh.model.getBoundingBox(-1,-1))
rows=[]
for d,t in vols:
    bb=gmsh.model.getBoundingBox(d,t); n=gmsh.model.getEntityName(d,t)
    rows.append((bb,n,t))
for bb,n,t in rows:
    sz=[bb[3]-bb[0],bb[4]-bb[1],bb[5]-bb[2]]
    print(t,n[:40],"min",[round(x,2) for x in bb[:3]],"size",[round(x,2) for x in sz])
