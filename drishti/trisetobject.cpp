#include "global.h"
#include "shaderfactory.h"
#include "staticfunctions.h"
#include "trisetobject.h"
#include "matrix.h"
#include "ply.h"
#include "matrix.h"
#include "volumeinformation.h"

#include "assimp/Importer.hpp"
#include "assimp/scene.h"
#include "assimp/postprocess.h"

#include <QFileDialog>

void
TrisetObject::gridSize(int &nx, int &ny, int &nz)
{
  nx = m_nX;
  ny = m_nY;
  nz = m_nZ;
}

void TrisetObject::setOpacity(float op) { m_opacity = op; }

TrisetObject::TrisetObject()
{
  QStringList ps;
  ps << "x";
  ps << "y";
  ps << "z";
  ps << "nx";
  ps << "ny";
  ps << "nz";
  ps << "red";
  ps << "green";
  ps << "blue";
  ps << "vertex_indices";
  ps << "vertex";
  ps << "face";

  for(int i=0; i<ps.count(); i++)
    {
      char *s;
      s = new char[ps[i].size()+1];
      strcpy(s, ps[i].toLatin1().data());
      plyStrings << s;
    }

  m_scrV = 0;
  m_scrD = 0;

  m_glVertBuffer = 0;
  m_glIndexBuffer = 0;
  m_glVertArray = 0;

  m_material.clear();

  clear();
}

TrisetObject::~TrisetObject() { clear(); }

void
TrisetObject::enclosingBox(Vec &boxMin,
			   Vec &boxMax)
{
  boxMin = m_enclosingBox[0];
  boxMax = m_enclosingBox[6];
}

void
TrisetObject::clear()
{
  m_show = true;
  m_clip = true;

  m_fileName.clear();
  m_centroid = Vec(0,0,0);
  m_position = Vec(0,0,0);
  m_scale = Vec(1,1,1);
  m_nX = m_nY = m_nZ = 0;
  m_color = Vec(0.7f,0.7f,0.7f);
  m_cropcolor = Vec(0.0f,0.0f,0.0f);
  m_opacity = 1.0f;
  m_specular = 1.0f;
  m_diffuse = 1.0f;
  m_ambient = 0.0f;
  m_pointMode = true;
  m_blendMode = false;
  m_shadows = false;
  m_flipNormals = false;
  m_screenDoor = false;
  m_pointSize = 5;
  m_pointStep = 1;
  m_vertices.clear();
  m_vcolor.clear();
  m_uv.clear();
  m_drawcolor.clear();
  m_normals.clear();
  m_triangles.clear();
  m_texValues.clear();
  m_tvertices.clear();
  m_tnormals.clear();

  if (m_scrV) delete [] m_scrV;
  if (m_scrD) delete [] m_scrD;
  m_scrV = 0;
  m_scrD = 0;

  if(m_glVertArray)
    {
      glDeleteBuffers(1, &m_glIndexBuffer);
      glDeleteVertexArrays( 1, &m_glVertArray );
      glDeleteBuffers(1, &m_glVertBuffer);
      m_glIndexBuffer = 0;
      m_glVertArray = 0;
      m_glVertBuffer = 0;
    }
  if (m_material.count() > 0)
    {
      glDeleteTextures(m_material.count(), m_diffuseTex);
      for(int d=0; d<m_material.count(); d++)
	m_diffuseTex[d] = 0;
    }
  m_material.clear();
  m_meshInfo.clear();
  
  m_featherSize = 1;
}

void
TrisetObject::setColor(Vec color)
{
  if (m_uv.count() > 0)
    {
      QMessageBox::information(0, "", "Cannot change color for textured objects");
      return;
    }
  
  m_color = color;

  for(int i=0; i<m_vcolor.count(); i++)
    m_vcolor[i] = m_color;
  
  loadVertexBufferData();
}

bool
TrisetObject::load(QString flnm)
{
  bool loaded;
  loaded = loadAssimpModel(flnm);

//  if (StaticFunctions::checkExtension(flnm, ".triset"))
//    loaded = loadTriset(flnm);
//  else
//    //loaded = loadPLY(flnm);
//    loaded = loadAssimpModel(flnm);

  m_featherSize = 0.005*(m_enclosingBox[6]-m_enclosingBox[0]).norm();

  
  if (loaded)
    {
      loadVertexBufferData();
      return true;
    }
  return false;
}

void
TrisetObject::loadVertexBufferData()
{
//  int stride = 1;
//  if (m_normals.count()) stride++; // per vertex normal
//  if (m_vcolor.count()) stride++; // per vertex color
//  
//  int nvert = m_vertices.count();
//  int nv = 3*stride*nvert;
//  int ni = m_triangles.count();
//  //---------------------
//
//  
//  //---------------------
//  float *vertData;
//  vertData = new float[nv];
//  for(int i=0; i<nvert; i++)
//    {
//      vertData[9*i + 0] = m_vertices[i].x;
//      vertData[9*i + 1] = m_vertices[i].y;
//      vertData[9*i + 2] = m_vertices[i].z;
//      vertData[9*i + 3] = m_normals[i].x;
//      vertData[9*i + 4] = m_normals[i].y;
//      vertData[9*i + 5] = m_normals[i].z;
//      vertData[9*i + 6] = m_vcolor[i].x;
//      vertData[9*i + 7] = m_vcolor[i].y;
//      vertData[9*i + 8] = m_vcolor[i].z;
//    }

  int stride = 3;
  if (m_normals.count()) stride += 3; // vertex normal
  if (m_uv.count() > 0 ||
      m_vcolor.count()) stride += 3; // vertex color or texture uv
  
  int nvert = m_vertices.count();
  int nv = stride*nvert;
  int ni = m_triangles.count();

  //---------------------
  float *vertData;
  vertData = new float[nv];  
  for(int i=0; i<nvert; i++)
    {
      vertData[stride*i + 0] = m_vertices[i].x;
      vertData[stride*i + 1] = m_vertices[i].y;
      vertData[stride*i + 2] = m_vertices[i].z;
      vertData[stride*i + 3] = m_normals[i].x;
      vertData[stride*i + 4] = m_normals[i].y;
      vertData[stride*i + 5] = m_normals[i].z;
    }

  if (m_uv.count() > 0)
    {
      for(int i=0; i<nvert; i++)
	{
	  vertData[stride*i + 6] = m_uv[i].x;
	  vertData[stride*i + 7] = m_uv[i].y;
	  vertData[stride*i + 8] = m_uv[i].z;
	}
    }
  else
    {
      for(int i=0; i<nvert; i++)
	{
	  vertData[stride*i + 6] = m_vcolor[i].x;
	  vertData[stride*i + 7] = m_vcolor[i].y;
	  vertData[stride*i + 8] = m_vcolor[i].z;
	}
    }


  unsigned int *indexData;
  indexData = new unsigned int[ni];
  for(int i=0; i<m_triangles.count(); i++)
    indexData[i] = m_triangles[i];
  //---------------------


  //--------------------
//  if (!m_diffuseTex)
//    glGenTextures(1, &m_diffuseTex);
//  glActiveTexture(GL_TEXTURE0);
//  glBindTexture(GL_TEXTURE_2D, m_diffuseTex);
//  glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE); 
//  glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE); 
//  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
//  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
//  QImage img(m_diffuseTexFile);
//  glTexImage2D(GL_TEXTURE_2D,
//	       0,
//	       4,
//	       img.width(),
//	       img.height(),
//	       0,
//	       GL_BGRA,
//	       GL_UNSIGNED_BYTE,
//	       img.bits());

  if (m_material.count() > 0)
    {
      glGenTextures(m_material.count(), m_diffuseTex);
      for(int d=0; d<m_material.count(); d++)
	{
	  if (!m_material[d].isEmpty())
	    {
	      glActiveTexture(GL_TEXTURE0);
	      glBindTexture(GL_TEXTURE_2D, m_diffuseTex[d]);
	      glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE); 
	      glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE); 
	      glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	      glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	      QImage img(m_material[d]);
	      glTexImage2D(GL_TEXTURE_2D,
			   0,
			   4,
			   img.width(),
			   img.height(),
			   0,
			   GL_BGRA,
			   GL_UNSIGNED_BYTE,
			   img.bits());
	    }
	  else if (m_uv.count() > 0)
	    {
	      glActiveTexture(GL_TEXTURE0);
	      glBindTexture(GL_TEXTURE_2D, m_diffuseTex[d]);
	      glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE); 
	      glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE); 
	      glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	      glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	      QImage img(1,1,QImage::Format_ARGB32);
	      img.fill(Qt::lightGray);
	      glTexImage2D(GL_TEXTURE_2D,
			   0,
			   4,
			   img.width(),
			   img.height(),
			   0,
			   GL_BGRA,
			   GL_UNSIGNED_BYTE,
			   img.bits());
	    }
	}
    }
  //---------------------
  
  
  if (!m_glVertArray)
    glGenVertexArrays(1, &m_glVertArray);
  glBindVertexArray(m_glVertArray);
      
  // Populate a vertex buffer
  if (!m_glVertBuffer)
    glGenBuffers(1, &m_glVertBuffer);
  glBindBuffer(GL_ARRAY_BUFFER, m_glVertBuffer);
  glBufferData(GL_ARRAY_BUFFER,
	       sizeof(float)*nv,
	       vertData,
	       GL_STATIC_DRAW);

  // Identify the components in the vertex buffer
  glEnableVertexAttribArray(0);
  glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE,
			sizeof(float)*stride, // stride
			(void *)0); // starting offset

  if (stride > 3)
    {
      glEnableVertexAttribArray(1);
      glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE,
			    sizeof(float)*stride,
			    (char *)NULL + sizeof(float)*3);
    }
  
  if (stride > 6)
    {
      glEnableVertexAttribArray(2);
      glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, 
			    sizeof(float)*stride,
			    (char *)NULL + sizeof(float)*6);
    }

  // Create and populate the index buffer
  if (!m_glIndexBuffer)
    glGenBuffers(1, &m_glIndexBuffer);
  glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_glIndexBuffer);
  glBufferData(GL_ELEMENT_ARRAY_BUFFER,
	       sizeof(unsigned int)*ni,
	       indexData,
	       GL_STATIC_DRAW);
  
  glBindVertexArray(0);

  delete [] vertData;
  delete [] indexData;

  // create shader  
  ShaderFactory::meshShader();
}

void
TrisetObject::drawTrisetBuffer(QGLViewer *viewer,
			       float pnear, float pfar)
{
  //glDisable(GL_BLEND);

  int ni = m_triangles.count();
  glBindVertexArray(m_glVertArray);
  glBindBuffer(GL_ARRAY_BUFFER, m_glVertBuffer);
  glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_glIndexBuffer);  
  glEnableVertexAttribArray(0);
  glEnableVertexAttribArray(1);
  glEnableVertexAttribArray(2);

  // model-view-projection matrix
  GLdouble m[16];
  GLfloat mvp[16];
  viewer->camera()->getModelViewProjectionMatrix(m);
  for(int i=0; i<16; i++) mvp[i] = m[i];

  Vec vd = viewer->camera()->viewDirection();

  //glUseProgram(ShaderFactory::meshShader());
  
  GLint *meshShaderParm = ShaderFactory::meshShaderParm();  

//  if (m_uv.count() > 0)
//    {
//      glActiveTexture(GL_TEXTURE0);
//      glEnable(GL_TEXTURE_2D);
//      glBindTexture(GL_TEXTURE_2D, m_diffuseTex);
//    }

  glUniformMatrix4fv(meshShaderParm[0], 1, GL_FALSE, mvp);
  glUniform3f(meshShaderParm[1], vd.x, vd.y, vd.z); // view direction
  glUniform3f(meshShaderParm[2], m_pn.x, m_pn.y, m_pn.z);
  glUniform1f(meshShaderParm[3], pnear);
  glUniform1f(meshShaderParm[4], pfar);
  glUniform1f(meshShaderParm[5], m_opacity);
  glUniform1f(meshShaderParm[6], m_ambient);
  glUniform1f(meshShaderParm[7], m_diffuse);
  glUniform1f(meshShaderParm[8], m_specular);

  glUniform3f(meshShaderParm[12], m_position.x, m_position.y, m_position.z);

//  if (m_uv.count() > 0)
//    {
//      glUniform1i(meshShaderParm[13], 1); // hasVY
//      glUniform1i(meshShaderParm[14], 0); // diffuseTex
//    }
//  else 
//    glUniform1i(meshShaderParm[13], 0); // no diffuse Tex

  glUniform1f(meshShaderParm[15], m_featherSize);

  
  //glDrawElements(GL_TRIANGLES, ni, GL_UNSIGNED_INT, 0);  

  for(int i=0; i<m_meshInfo.count(); i++)
    {
      QPolygon poly = m_meshInfo[i];
      int vStart = poly.point(0).x();
      int vEnd = poly.point(0).y();
      int idxStart = poly.point(1).x();
      int idxEnd = poly.point(1).y();
      int matIdx = poly.point(2).x();
      int nTri = idxEnd-idxStart+1;
      
      //if (! m_material[matIdx].isEmpty())
      if (m_uv.count() > 0)
	{
	  glActiveTexture(GL_TEXTURE0);
	  glEnable(GL_TEXTURE_2D);
	  glBindTexture(GL_TEXTURE_2D, m_diffuseTex[matIdx]);
	  glUniform1i(meshShaderParm[13], 1); // hasVY
	  glUniform1i(meshShaderParm[14], 0); // diffuseTex
	}
      else
	glUniform1i(meshShaderParm[13], 0); // no diffuse texture
      
      glDrawElements(GL_TRIANGLES, nTri, GL_UNSIGNED_INT, (char *)NULL + idxStart*sizeof(unsigned int));
    }
  
  
  glDisable(GL_TEXTURE_2D);
  //if (m_uv.count() > 0) glDisable(GL_TEXTURE_2D);

  glDisableVertexAttribArray(0);
  glDisableVertexAttribArray(1);
  glDisableVertexAttribArray(2);
  glBindVertexArray(0);
  
  glUseProgram(0);
}

bool
TrisetObject::loadPLY(QString flnm)
{
  m_position = Vec(0,0,0);
  m_scale = Vec(1,1,1);

  typedef struct Vertex {
    float x,y,z;
    float r,g,b;
    float nx,ny,nz;
    void *other_props;       /* other properties */
  } Vertex;

  typedef struct Face {
    unsigned char nverts;    /* number of vertex indices in list */
    int *verts;              /* vertex index list */
    void *other_props;       /* other properties */
  } Face;

  PlyProperty vert_props[] = { /* list of property information for a vertex */
    {plyStrings[0], Float32, Float32, offsetof(Vertex,x), 0, 0, 0, 0},
    {plyStrings[1], Float32, Float32, offsetof(Vertex,y), 0, 0, 0, 0},
    {plyStrings[2], Float32, Float32, offsetof(Vertex,z), 0, 0, 0, 0},
    {plyStrings[6], Float32, Float32, offsetof(Vertex,r), 0, 0, 0, 0},
    {plyStrings[7], Float32, Float32, offsetof(Vertex,g), 0, 0, 0, 0},
    {plyStrings[8], Float32, Float32, offsetof(Vertex,b), 0, 0, 0, 0},
    {plyStrings[3], Float32, Float32, offsetof(Vertex,nx), 0, 0, 0, 0},
    {plyStrings[4], Float32, Float32, offsetof(Vertex,ny), 0, 0, 0, 0},
    {plyStrings[5], Float32, Float32, offsetof(Vertex,nz), 0, 0, 0, 0},
  };

  PlyProperty face_props[] = { /* list of property information for a face */
    {plyStrings[9], Int32, Int32, offsetof(Face,verts),
     1, Uint8, Uint8, offsetof(Face,nverts)},
  };


  /*** the PLY object ***/

  int nverts,nfaces;
  Vertex **vlist;
  Face **flist;

  PlyOtherProp *vert_other,*face_other;

  bool per_vertex_color = false;
  bool has_normals = false;

  int i,j;
  int elem_count;
  char *elem_name;
  PlyFile *in_ply;


  /*** Read in the original PLY object ***/
  FILE *fp = fopen(flnm.toLatin1().data(), "rb");

  in_ply  = read_ply (fp);

  for (i = 0; i < in_ply->num_elem_types; i++) {

    /* prepare to read the i'th list of elements */
    elem_name = setup_element_read_ply (in_ply, i, &elem_count);


    if (QString("vertex") == QString(elem_name)) {

      /* create a vertex list to hold all the vertices */
      vlist = (Vertex **) malloc (sizeof (Vertex *) * elem_count);
      nverts = elem_count;

      /* set up for getting vertex elements */

      setup_property_ply (in_ply, &vert_props[0]);
      setup_property_ply (in_ply, &vert_props[1]);
      setup_property_ply (in_ply, &vert_props[2]);

      for (j = 0; j < in_ply->elems[i]->nprops; j++) {
	PlyProperty *prop;
	prop = in_ply->elems[i]->props[j];
	if (QString("r") == QString(prop->name) ||
	    QString("red") == QString(prop->name)) {
	  setup_property_ply (in_ply, &vert_props[3]);
	  per_vertex_color = true;
	}
	if (QString("g") == QString(prop->name) ||
	    QString("green") == QString(prop->name)) {
	  setup_property_ply (in_ply, &vert_props[4]);
	  per_vertex_color = true;
	}
	if (QString("b") == QString(prop->name) ||
	    QString("blue") == QString(prop->name)) {
	  setup_property_ply (in_ply, &vert_props[5]);
	  per_vertex_color = true;
	}
	if (QString("nx") == QString(prop->name)) {
	  setup_property_ply (in_ply, &vert_props[6]);
	  has_normals = true;
	}
	if (QString("ny") == QString(prop->name)) {
	  setup_property_ply (in_ply, &vert_props[7]);
	  has_normals = true;
	}
	if (QString("nz") == QString(prop->name)) {
	  setup_property_ply (in_ply, &vert_props[8]);
	  has_normals = true;
	}
      }

      vert_other = get_other_properties_ply (in_ply, 
					     offsetof(Vertex,other_props));

      /* grab all the vertex elements */
      for (j = 0; j < elem_count; j++) {
        vlist[j] = (Vertex *) malloc (sizeof (Vertex));
        get_element_ply (in_ply, (void *) vlist[j]);
      }
    }
    else if (QString("face") == QString(elem_name)) {

      /* create a list to hold all the face elements */
      flist = (Face **) malloc (sizeof (Face *) * elem_count);
      nfaces = elem_count;

      /* set up for getting face elements */

      setup_property_ply (in_ply, &face_props[0]);
      face_other = get_other_properties_ply (in_ply, 
					     offsetof(Face,other_props));

      /* grab all the face elements */
      for (j = 0; j < elem_count; j++) {
        flist[j] = (Face *) malloc (sizeof (Face));
        get_element_ply (in_ply, (void *) flist[j]);
      }
    }
    else
      get_other_element_ply (in_ply);
  }

  close_ply (in_ply);
  free_ply (in_ply);
  
  if (Global::volumeType() == Global::DummyVolume)
    {
      float minX, maxX;
      float minY, maxY;
      float minZ, maxZ;
      minX = maxX = vlist[0]->x;
      minY = maxY = vlist[0]->y;
      minZ = maxZ = vlist[0]->z;
      for(int i=0; i<nverts; i++)
	{
	  minX = qMin(minX, vlist[i]->x);
	  maxX = qMax(maxX, vlist[i]->x);
	  minY = qMin(minY, vlist[i]->y);
	  maxY = qMax(maxY, vlist[i]->y);
	  minZ = qMin(minZ, vlist[i]->z);
	  maxZ = qMax(maxZ, vlist[i]->z);
	}
      minX = floor(minX);
      minY = floor(minY);
      minZ = floor(minZ);
      maxX = ceil(maxX);
      maxY = ceil(maxY);
      maxZ = ceil(maxZ);
      int h = maxX-minX+1;
      int w = maxY-minY+1;
      int d = maxZ-minZ+1;

      m_nX = d;
      m_nY = w;
      m_nZ = h;
      //m_position = Vec(-minX, -minY, -minZ);

      
//      bool ok;
//      QString text = QInputDialog::getText(0,
//					   "Please enter grid size",
//					   "Grid Size",
//					   QLineEdit::Normal,
//					   QString("%1 %2 %3").\
//					   arg(d).arg(w).arg(h),
//					   &ok);
//      if (!ok || text.isEmpty())
//	{
//	  QMessageBox::critical(0, "Cannot load PLY", "No grid");
//	  return false;
//	}
//      
//      int nx=0;
//      int ny=0;
//      int nz=0;
//      QStringList gs = text.split(" ", QString::SkipEmptyParts);
//      if (gs.count() > 0) nx = gs[0].toInt();
//      if (gs.count() > 1) ny = gs[1].toInt();
//      if (gs.count() > 2) nz = gs[2].toInt();
//      if (nx > 0 && ny > 0 && nz > 0)
//	{
//	  m_nX = nx;
//	  m_nY = ny;
//	  m_nZ = nz;
//	}
//      else
//	{
//	  QMessageBox::critical(0, "Cannot load triset", "No grid");
//	  return false;
//	}
//
//      if (d == m_nX && w == m_nY && h == m_nZ)
//	m_position = Vec(-minX, -minY, -minZ);
    }
  else
    {
      Vec dim = VolumeInformation::volumeInformation(0).dimensions;
      m_nZ = dim.x;
      m_nY = dim.y;
      m_nX = dim.z;
    }

  m_vertices.resize(nverts);
  for(int i=0; i<nverts; i++)
    m_vertices[i] = Vec(vlist[i]->x,
			vlist[i]->y,
			vlist[i]->z);


  m_normals.clear();
  if (has_normals)
    {
      m_normals.resize(nverts);
      for(int i=0; i<nverts; i++)
	m_normals[i] = Vec(vlist[i]->nx,
			   vlist[i]->ny,
			   vlist[i]->nz);
    }

  m_vcolor.clear();
  if (per_vertex_color)
    {
      m_vcolor.resize(nverts);
      for(int i=0; i<nverts; i++)
	m_vcolor[i] = Vec(vlist[i]->r/255.0f,
			  vlist[i]->g/255.0f,
			  vlist[i]->b/255.0f);
    }

  // only triangles considered
  int ntri=0;
  for (int i=0; i<nfaces; i++)
    {
      if (flist[i]->nverts >= 3)
	ntri++;
    }
  m_triangles.resize(3*ntri);

  int tri=0;
  for(int i=0; i<nfaces; i++)
    {
      if (flist[i]->nverts >= 3)
	{
	  m_triangles[3*tri+0] = flist[i]->verts[0];
	  m_triangles[3*tri+1] = flist[i]->verts[1];
	  m_triangles[3*tri+2] = flist[i]->verts[2];
	  tri++;
	}
    }



  m_tvertices.resize(nverts);
  m_tnormals.resize(nverts);
  m_texValues.resize(nverts);


  Vec bmin = m_vertices[0];
  Vec bmax = m_vertices[0];
  for(int i=0; i<nverts; i++)
    {
      bmin = StaticFunctions::minVec(bmin, m_vertices[i]);
      bmax = StaticFunctions::maxVec(bmax, m_vertices[i]);
    }
  m_centroid = (bmin + bmax)/2;

  m_enclosingBox[0] = Vec(bmin.x, bmin.y, bmin.z);
  m_enclosingBox[1] = Vec(bmax.x, bmin.y, bmin.z);
  m_enclosingBox[2] = Vec(bmax.x, bmax.y, bmin.z);
  m_enclosingBox[3] = Vec(bmin.x, bmax.y, bmin.z);
  m_enclosingBox[4] = Vec(bmin.x, bmin.y, bmax.z);
  m_enclosingBox[5] = Vec(bmax.x, bmin.y, bmax.z);
  m_enclosingBox[6] = Vec(bmax.x, bmax.y, bmax.z);
  m_enclosingBox[7] = Vec(bmin.x, bmax.y, bmax.z);


  m_pointStep = qMax(1, nverts/50000);

//  QMessageBox::information(0, "", QString("%1 %2 %3\n%4 %5").	\
//			   arg(m_nX).arg(m_nY).arg(m_nZ).	\
//			   arg(m_vertices.count()).		\
//			   arg(m_triangles.count()/3));


  m_fileName = flnm;

  return true;
}

bool
TrisetObject::loadTriset(QString flnm)
{
  QFile fd(flnm);
  fd.open(QFile::ReadOnly);

  uchar stype = 0;
  fd.read((char*)&stype, sizeof(uchar));
  if (stype != 0)
    {
      QMessageBox::critical(0, "Cannot load triset",
			    "Wrong input format : First byte not equal to 0");
      return false;
    }

  fd.read((char*)&m_nX, sizeof(int));
  fd.read((char*)&m_nY, sizeof(int));
  fd.read((char*)&m_nZ, sizeof(int));


  int nvert, ntri;
  fd.read((char*)&nvert, sizeof(int));
  fd.read((char*)&ntri, sizeof(int));
   

//  QMessageBox::information(0, "", QString("%1 %2 %3\n%4 %5").	\
//			   arg(m_nX).arg(m_nY).arg(m_nZ).\
//			   arg(nvert).\
//			   arg(ntri));


  float *vert = new float[nvert*3];
  fd.read((char*)vert, sizeof(float)*3*nvert);

  float *vnorm = new float[nvert*3];
  fd.read((char*)vnorm, sizeof(float)*3*nvert);

  int *tri = new int[ntri*3];
  fd.read((char*)tri, sizeof(int)*3*ntri);

  fd.close();


  m_vertices.resize(nvert);
  for(int i=0; i<nvert; i++)
    m_vertices[i] = Vec(vert[3*i],
			vert[3*i+1],
			vert[3*i+2]);
  delete [] vert;

  m_normals.resize(nvert);
  for(int i=0; i<nvert; i++)
    m_normals[i] = Vec(vnorm[3*i],
		       vnorm[3*i+1],
		       vnorm[3*i+2]);
  delete [] vnorm;

  m_triangles.resize(3*ntri);
  for(int i=0; i<3*ntri; i++)
    m_triangles[i] = tri[i];
  delete [] tri;


  m_tvertices.resize(nvert);
  m_tnormals.resize(nvert);
  m_texValues.resize(nvert);


  Vec bmin = m_vertices[0];
  Vec bmax = m_vertices[0];
  for(int i=0; i<nvert; i++)
    {
      bmin = StaticFunctions::minVec(bmin, m_vertices[i]);
      bmax = StaticFunctions::maxVec(bmax, m_vertices[i]);
    }
  m_centroid = (bmin + bmax)/2;

  m_position = Vec(0,0,0);
  m_scale = Vec(1,1,1);

  m_enclosingBox[0] = Vec(bmin.x, bmin.y, bmin.z);
  m_enclosingBox[1] = Vec(bmax.x, bmin.y, bmin.z);
  m_enclosingBox[2] = Vec(bmax.x, bmax.y, bmin.z);
  m_enclosingBox[3] = Vec(bmin.x, bmax.y, bmin.z);
  m_enclosingBox[4] = Vec(bmin.x, bmin.y, bmax.z);
  m_enclosingBox[5] = Vec(bmax.x, bmin.y, bmax.z);
  m_enclosingBox[6] = Vec(bmax.x, bmax.y, bmax.z);
  m_enclosingBox[7] = Vec(bmin.x, bmax.y, bmax.z);


  m_pointStep = qMax(1, nvert/50000);

//  QMessageBox::information(0, "", QString("%1 %2 %3\n%4 %5").	\
//			   arg(m_nX).arg(m_nY).arg(m_nZ).\
//			   arg(m_vertices.count()).\
//			   arg(m_triangles.count()));

  m_fileName = flnm;

  return true;
}


void
TrisetObject::postdraw(QGLViewer *viewer,
		       int x, int y,
		       bool active, int idx)
{
  if (!m_show || !active)
    return;

  viewer->startScreenCoordinatesSystem();

  glEnable(GL_BLEND);
  glBlendFunc(GL_ONE, GL_ONE_MINUS_SRC_ALPHA); // blend on top

  QString str = QString("triset %1").arg(idx);
  str += QString(" (%1)").arg(QFileInfo(m_fileName).fileName());
  QFont font = QFont();
  QFontMetrics metric(font);
  int ht = metric.height();
  int wd = metric.width(str);
  //x -= wd/2;
  x += 10;

  StaticFunctions::renderText(x+2, y, str, font, Qt::black, Qt::white);

  viewer->stopScreenCoordinatesSystem();
}

void
TrisetObject::draw(QGLViewer *viewer,
		   bool active,
		   Vec lightPosition,
		   float pnear, float pfar, Vec step)
{

  StaticFunctions::drawEnclosingCube(m_tenclosingBox, Vec(1,1,0.5));

  if (!m_show)
    return;
  
  if (active)
    {
      Vec lineColor = Vec(0.7f, 0.3f, 0);
      StaticFunctions::drawEnclosingCube(m_tenclosingBox, lineColor);
    }
  
  if (m_opacity < 0.05)
    return;

  float pos[4];
  float amb[4];
  float diff[4];
  float spec[4];
  float shine = 128*m_specular;

  glEnable(GL_LIGHTING);
  glShadeModel(GL_SMOOTH);
  glEnable(GL_LIGHT0);

  //glLightModeli(GL_LIGHT_MODEL_LOCAL_VIEWER, GL_TRUE); // assume infinite view point
  //glLightModeli(GL_LIGHT_MODEL_TWO_SIDE, GL_TRUE);

  for (int i=0; i<4; i++)
    spec[i] = m_specular;

  for (int i=0; i<4; i++)
    diff[i] = m_diffuse;

  for (int i=0; i<4; i++)
    amb[i] = m_ambient;


  glMaterialfv(GL_FRONT_AND_BACK, GL_SPECULAR, spec);
  glMaterialfv(GL_FRONT_AND_BACK, GL_SHININESS, &shine);

  // emissive when active
  if (active)
    {
      float emiss[] = { 0.5f, 0, 0, 1 };
      glMaterialfv(GL_FRONT_AND_BACK, GL_EMISSION, emiss);
    }
  else
    {
      float emiss[] = { 0, 0, 0, 1 };
      glMaterialfv(GL_FRONT_AND_BACK, GL_EMISSION, emiss);
    }


  glLightfv(GL_LIGHT0, GL_DIFFUSE, diff);
  glLightfv(GL_LIGHT0, GL_AMBIENT, amb);
  if (spec > 0)
    glLightfv(GL_LIGHT0, GL_SPECULAR, spec);

  glColorMaterial(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE);
  glEnable(GL_COLOR_MATERIAL);

  pos[0] = lightPosition.x;
  pos[1] = lightPosition.y;
  pos[2] = lightPosition.z;
  pos[3] = 0;
  glLightfv(GL_LIGHT0, GL_POSITION, pos);

  glColor4f(m_color.x*m_opacity,
	    m_color.y*m_opacity,
	    m_color.z*m_opacity,
	    m_opacity);

  if (m_blendMode)
    drawTrisetBuffer(viewer, pnear, pfar);
  else
    drawTrisetBuffer(viewer, 0, -1);

  { // reset emissivity
    float emiss[] = { 0, 0, 0, 1 };
    glMaterialfv(GL_FRONT_AND_BACK, GL_EMISSION, emiss);
  }


  glDisable(GL_LIGHTING);
}

void
TrisetObject::predraw(QGLViewer *viewer,
		      double *Xform,
		      Vec pn,
		      bool shadows, int shadowWidth, int shadowHeight)
{
  m_pn = pn;

  if (m_opacity < 0.05)
    return;

  double *sp0 = new double[16];
  double *sp1 = new double[16];
  double *scl = new double[16];
  Matrix::identity(sp0);
  sp0[3] = m_centroid.x;
  sp0[7] = m_centroid.y;
  sp0[11]= m_centroid.z;

  Matrix::identity(scl);
  scl[0] = m_scale.x;
  scl[5] = m_scale.y;
  scl[10] = m_scale.z;
  Matrix::matmult(sp0,scl,sp1);  // sp1 = sp0 * scl

  Matrix::identity(sp0);
  sp0[3] = -m_centroid.x;
  sp0[7] = -m_centroid.y;
  sp0[11]= -m_centroid.z;  
  Matrix::matmult(sp1, sp0, scl); // scl = sp1 * sp0

  double *localXform = new double[16];
  Matrix::matmult(scl, Xform, sp1);

  Matrix::identity(sp0);
  sp0[3] = m_position.x;
  sp0[7] = m_position.y;
  sp0[11]= m_position.z;
  Matrix::matmult(sp1, sp0, localXform);

  delete [] sp0;
  delete [] sp1;
  delete [] scl;


  m_tcentroid = Matrix::xformVec(localXform, m_centroid);

  for(int i=0; i<8; i++)
    m_tenclosingBox[i] = Matrix::xformVec(localXform, m_enclosingBox[i]);

  for(int i=0; i<m_vertices.count(); i++)
    m_tvertices[i] = Matrix::xformVec(localXform, m_vertices[i]);


  if (m_normals.count() > 0)
    {  int fn = 1;
      if (m_flipNormals)
	fn = -1;
      for(int i=0; i<m_normals.count(); i++)
	m_tnormals[i] = Matrix::rotateVec(localXform, (fn*m_normals[i]));
    }

  if (m_blendMode)
    {
      if (!shadows || !m_shadows)
	{
	  for(int i=0; i<m_vertices.count(); i++)
	    m_texValues[i] = Vec(pn*m_tvertices[i], 0, 0);
	}
      else
	{
	  for(int i=0; i<m_vertices.count(); i++)
	    {
	      Vec scr = viewer->camera()->projectedCoordinatesOf(m_tvertices[i]);
	      float tx = scr.x;
	      float ty = shadowHeight-scr.y;
	      
	      float d = pn * m_tvertices[i];
	      
	      m_texValues[i] = Vec(d, tx, ty);
	    }
	}
    }

  bool black = (m_color.x<0.1 && m_color.y<0.1 && m_color.z<0.1);
  if (black)
    {
      m_drawcolor.clear();
      m_drawcolor = m_vcolor;
      for(int i=0; i<m_drawcolor.count(); i++)
	m_drawcolor[i] *= m_opacity;
    }


  delete [] localXform;
}

void
TrisetObject::makeReadyForPainting(QGLViewer *viewer)
{
  if (m_scrV) delete [] m_scrV;
  if (m_scrD) delete [] m_scrD;

  int swd = viewer->camera()->screenWidth();
  int sht = viewer->camera()->screenHeight();
  m_scrV = new uint[swd*sht];
  m_scrD = new float[swd*sht];

  for(int i=0; i<swd*sht; i++)
    m_scrD[i] = -1;
  for(int i=0; i<swd*sht; i++)
    m_scrV[i] = 1000000000; // a very large number - we will not have billion vertices

  for(int i=0; i<m_tvertices.count(); i++)
    {
      Vec scr = viewer->camera()->projectedCoordinatesOf(m_tvertices[i]);
      int tx = scr.x;
      int ty = sht-scr.y;
      if (tx>0 && tx<swd && ty>0 && ty<sht)
	{
	  if (scr.z > 0.0 && scr.z > m_scrD[tx*sht+ty])
	    {
	      m_scrV[tx*sht + ty] = i;
	      m_scrD[tx*sht + ty] = scr.z;
	    }
	}
    }

  if (m_vcolor.count() == 0)
    { // create per vertex color and fill with white
      m_vcolor.resize(m_vertices.count());
      m_vcolor.fill(Vec(1,1,1));
    }

  bool black = (m_color.x<0.1 && m_color.y<0.1 && m_color.z<0.1);
  if (!black) // make it black so that actual vertex colors are displayed
    m_color = Vec(0,0,0);
}

void
TrisetObject::releaseFromPainting()
{
  if (m_scrV) delete [] m_scrV;
  if (m_scrD) delete [] m_scrD;
  m_scrV = 0;
  m_scrD = 0;
}

void
TrisetObject::paint(QGLViewer *viewer,
		    QBitArray doodleMask,
		    float *depthMap,
		    Vec tcolor, float tmix)
{
  int swd = viewer->camera()->screenWidth();
  int sht = viewer->camera()->screenHeight();

  for(int i=0; i<m_tvertices.count(); i++)
    {
      Vec scr = viewer->camera()->projectedCoordinatesOf(m_tvertices[i]);
      int tx = scr.x;
      int ty = sht-scr.y;
      float td = scr.z;
      if (tx>0 && tx<swd && ty>0 && ty<sht)
	{
	  int idx = ty*swd + tx;
	  if (doodleMask.testBit(idx))
	    {
	      float zd = depthMap[idx];
	      if (fabs(zd-td) < 0.0002)
		m_vcolor[i] = tmix*tcolor + (1.0-tmix)*m_vcolor[i];
	    }
	}
    }
}

QDomElement
TrisetObject::domElement(QDomDocument &doc)
{
  QDomElement de = doc.createElement("triset");
  
  {
    QDomElement de0 = doc.createElement("name");
    QDomText tn0 = doc.createTextNode(m_fileName);
    de0.appendChild(tn0);
    de.appendChild(de0);
  }
  
  {
    QDomElement de0 = doc.createElement("position");
    QDomText tn0 = doc.createTextNode(QString("%1 %2 %3").\
				      arg(m_position.x).arg(m_position.y).arg(m_position.z));
    de0.appendChild(tn0);
    de.appendChild(de0);
  }
  
  {
    QDomElement de0 = doc.createElement("scale");
    QDomText tn0 = doc.createTextNode(QString("%1 %2 %3").\
				      arg(m_scale.x).arg(m_scale.y).arg(m_scale.z));
    de0.appendChild(tn0);
    de.appendChild(de0);
  }
  
  {
    QDomElement de0 = doc.createElement("opacity");
    QDomText tn0 = doc.createTextNode(QString("%1").arg(m_opacity));
    de0.appendChild(tn0);
    de.appendChild(de0);
  }
  
  {
    QDomElement de0 = doc.createElement("color");
    QDomText tn0 = doc.createTextNode(QString("%1 %2 %3").\
				      arg(m_color.x).arg(m_color.y).arg(m_color.z));
    de0.appendChild(tn0);
    de.appendChild(de0);
  }
  
  {
    QDomElement de0 = doc.createElement("cropcolor");
    QDomText tn0 = doc.createTextNode(QString("%1 %2 %3").\
				      arg(m_cropcolor.x).arg(m_cropcolor.y).arg(m_cropcolor.z));
    de0.appendChild(tn0);
    de.appendChild(de0);
  }
  
  {
    QDomElement de0 = doc.createElement("ambient");
    QDomText tn0 = doc.createTextNode(QString("%1").arg(m_ambient));
    de0.appendChild(tn0);
    de.appendChild(de0);
  }
  
  {
    QDomElement de0 = doc.createElement("diffuse");
    QDomText tn0 = doc.createTextNode(QString("%1").arg(m_diffuse));
    de0.appendChild(tn0);
    de.appendChild(de0);
  }
  
  {
    QDomElement de0 = doc.createElement("specular");
    QDomText tn0 = doc.createTextNode(QString("%1").arg(m_specular));
    de0.appendChild(tn0);
    de.appendChild(de0);
  }

  {
    QDomElement de0 = doc.createElement("pointmode");
    QDomText tn0 = doc.createTextNode(QString("%1").arg(m_pointMode));
    de0.appendChild(tn0);
    de.appendChild(de0);
  }

  {
    QDomElement de0 = doc.createElement("pointsize");
    QDomText tn0 = doc.createTextNode(QString("%1").arg(m_pointSize));
    de0.appendChild(tn0);
    de.appendChild(de0);
  }
  
  {
    QDomElement de0 = doc.createElement("pointstep");
    QDomText tn0 = doc.createTextNode(QString("%1").arg(m_pointStep));
    de0.appendChild(tn0);
    de.appendChild(de0);
  }
  
  {
    QDomElement de0 = doc.createElement("blendmode");
    QDomText tn0 = doc.createTextNode(QString("%1").arg(m_blendMode));
    de0.appendChild(tn0);
    de.appendChild(de0);
  }

  {
    QDomElement de0 = doc.createElement("flipnormals");
    QDomText tn0 = doc.createTextNode(QString("%1").arg(m_flipNormals));
    de0.appendChild(tn0);
    de.appendChild(de0);
  }
  
  {
    QDomElement de0 = doc.createElement("screendoor");
    QDomText tn0 = doc.createTextNode(QString("%1").arg(m_screenDoor));
    de0.appendChild(tn0);
    de.appendChild(de0);
  }
  
  {
    QDomElement de0 = doc.createElement("show");
    QDomText tn0 = doc.createTextNode(QString("%1").arg(m_show));
    de0.appendChild(tn0);
    de.appendChild(de0);
  }

  {
    QDomElement de0 = doc.createElement("clip");
    QDomText tn0 = doc.createTextNode(QString("%1").arg(m_clip));
    de0.appendChild(tn0);
    de.appendChild(de0);
  }

  return de;
}

bool
TrisetObject::fromDomElement(QDomElement de)
{
  clear();

  bool ok = false;

  QString name;
  QDomNodeList dlist = de.childNodes();
  for(int i=0; i<dlist.count(); i++)
    {
      QDomElement dnode = dlist.at(i).toElement();
      QString str = dnode.toElement().text();
      if (dnode.tagName() == "name")
	ok = load(str);
      else if (dnode.tagName() == "position")
	{
	  QStringList xyz = str.split(" ");
	  float x = 0;
	  float y = 0;
	  float z = 0;
	  if (xyz.size() > 0) x = xyz[0].toFloat();
	  if (xyz.size() > 1) y  = xyz[1].toFloat();
	  if (xyz.size() > 2) z  = xyz[2].toFloat();
	  m_position = Vec(x,y,z);
	}
      else if (dnode.tagName() == "scale")
	{
	  QStringList xyz = str.split(" ");
	  float x = 0;
	  float y = 0;
	  float z = 0;
	  if (xyz.size() > 0) x = xyz[0].toFloat();
	  if (xyz.size() > 1) y  = xyz[1].toFloat();
	  if (xyz.size() > 2) z  = xyz[2].toFloat();
	  m_scale = Vec(x,y,z);
	}
      else if (dnode.tagName() == "opacity")
	m_opacity = str.toFloat();
      else if (dnode.tagName() == "color")
	{
	  QStringList xyz = str.split(" ");
	  float x = 0;
	  float y = 0;
	  float z = 0;
	  if (xyz.size() > 0) x = xyz[0].toFloat();
	  if (xyz.size() > 1) y  = xyz[1].toFloat();
	  if (xyz.size() > 2) z  = xyz[2].toFloat();
	  m_color = Vec(x,y,z);
	}
      else if (dnode.tagName() == "cropcolor")
	{
	  QStringList xyz = str.split(" ");
	  float x = 0;
	  float y = 0;
	  float z = 0;
	  if (xyz.size() > 0) x = xyz[0].toFloat();
	  if (xyz.size() > 1) y  = xyz[1].toFloat();
	  if (xyz.size() > 2) z  = xyz[2].toFloat();
	  m_cropcolor = Vec(x,y,z);
	}
      else if (dnode.tagName() == "ambient")
	m_ambient = str.toFloat();
      else if (dnode.tagName() == "diffuse")
	m_diffuse = str.toFloat();
      else if (dnode.tagName() == "specular")
	m_specular = str.toFloat();
      else if (dnode.tagName() == "pointmode")
	{
	  if (str == "yes" || str == "1") m_pointMode = true;
	  else m_pointMode = false;
	}
      else if (dnode.tagName() == "pointsize")
	m_pointSize = str.toFloat();
      else if (dnode.tagName() == "pointstep")
	m_pointStep = str.toFloat();
      else if (dnode.tagName() == "blendmode")
	{
	  if (str == "yes" || str == "1") m_pointMode = true;
	  else m_blendMode = false;
	}
      else if (dnode.tagName() == "screendoor")
	{
	  if (str == "yes" || str == "1") m_screenDoor = true;
	  else m_screenDoor = false;
	}
      else if (dnode.tagName() == "flipnormals")
	{
	  if (str == "yes" || str == "1") m_flipNormals = true;
	  else m_flipNormals = false;
	}
      else if (dnode.tagName() == "show")
	{
	  if (str == "yes" || str == "1") m_show = true;
	  else m_show = false;
	}
      else if (dnode.tagName() == "clip")
	{
	  if (str == "yes" || str == "1") m_clip = true;
	  else m_clip = false;
	}
    }

  return ok;
}

TrisetInformation
TrisetObject::get()
{
  TrisetInformation ti;
  ti.show = m_show;
  ti.clip = m_clip;
  ti.filename = m_fileName;
  ti.position = m_position;
  ti.scale = m_scale;
  ti.pointMode = m_pointMode;
  ti.blendMode = m_blendMode;
  ti.shadows = m_shadows;
  ti.pointStep = m_pointStep;
  ti.pointSize = m_pointSize;
  ti.color = m_color;
  ti.cropcolor = m_cropcolor;
  ti.opacity = m_opacity;
  ti.ambient = m_ambient;
  ti.diffuse = m_diffuse;
  ti.specular = m_specular;
  ti.flipNormals = m_flipNormals;
  ti.screenDoor = m_screenDoor;

  return ti;
}

bool
TrisetObject::set(TrisetInformation ti)
{
  bool ok = false;

  if (m_fileName != ti.filename)
    ok = load(ti.filename);
  else
    ok = true;

  bool reloadColor = false;
  if (m_color != ti.color)
    {
      reloadColor = true;
    }
  
  m_show = ti.show;
  m_clip = ti.clip;
  m_position = ti.position;
  m_scale = ti.scale;
  m_opacity = ti.opacity;
  m_color = ti.color;
  m_cropcolor = ti.cropcolor;
  m_ambient = ti.ambient;
  m_diffuse = ti.diffuse;
  m_specular = ti.specular;
  m_pointMode = ti.pointMode;
  m_pointSize = ti.pointSize;
  m_pointStep = ti.pointStep;
  m_blendMode = ti.blendMode;
  m_shadows = ti.shadows;
  m_flipNormals = ti.flipNormals;
  m_screenDoor = ti.screenDoor;


  if (reloadColor)
    setColor(m_color);
    
  return ok;
}

void
TrisetObject::save()
{
  bool has_normals = (m_normals.count() > 0);
  bool per_vertex_color = (m_vcolor.count() > 0);

  QString flnm = QFileDialog::getSaveFileName(0,
					      "Export mesh to file",
					      Global::previousDirectory(),
					      "*.ply");
  if (flnm.size() == 0)
    return;

  typedef struct PlyFace
  {
    unsigned char nverts;    /* number of Vertex indices in list */
    int *verts;              /* Vertex index list */
  } PlyFace;

  typedef struct
  {
    float  x,  y,  z;  /**< Vertex coordinates */
    float nx, ny, nz;  /**< Vertex normal */
    uchar r, g, b;
  } myVertex ;


  PlyProperty vert_props[] = { /* list of property information for a vertex */
    {plyStrings[0], Float32, Float32, offsetof(myVertex,x), 0, 0, 0, 0},
    {plyStrings[1], Float32, Float32, offsetof(myVertex,y), 0, 0, 0, 0},
    {plyStrings[2], Float32, Float32, offsetof(myVertex,z), 0, 0, 0, 0},
    {plyStrings[3], Float32, Float32, offsetof(myVertex,nx), 0, 0, 0, 0},
    {plyStrings[4], Float32, Float32, offsetof(myVertex,ny), 0, 0, 0, 0},
    {plyStrings[5], Float32, Float32, offsetof(myVertex,nz), 0, 0, 0, 0},
    {plyStrings[6], Uint8, Uint8, offsetof(myVertex,r), 0, 0, 0, 0},
    {plyStrings[7], Uint8, Uint8, offsetof(myVertex,g), 0, 0, 0, 0},
    {plyStrings[8], Uint8, Uint8, offsetof(myVertex,b), 0, 0, 0, 0},
  };

  PlyProperty face_props[] = { /* list of property information for a face */
    {plyStrings[9], Int32, Int32, offsetof(PlyFace,verts),
     1, Uint8, Uint8, offsetof(PlyFace,nverts)},
  };

  PlyFile    *ply;
  FILE       *fp = fopen(flnm.toLatin1().data(), bin ? "wb" : "w");

  PlyFace     face ;
  int         verts[3] ;
  char       *elem_names[]  = {plyStrings[10], plyStrings[11]};
  ply = write_ply (fp,
		   2,
		   elem_names,
		   bin ? PLY_BINARY_LE : PLY_ASCII );

  int nvertices = m_vertices.count();
  /* describe what properties go into the PlyVertex elements */
  describe_element_ply ( ply, plyStrings[10], nvertices );
  describe_property_ply ( ply, &vert_props[0] );
  describe_property_ply ( ply, &vert_props[1] );
  describe_property_ply ( ply, &vert_props[2] );
  describe_property_ply ( ply, &vert_props[3] );
  describe_property_ply ( ply, &vert_props[4] );
  describe_property_ply ( ply, &vert_props[5] );
  describe_property_ply ( ply, &vert_props[6] );
  describe_property_ply ( ply, &vert_props[7] );
  describe_property_ply ( ply, &vert_props[8] );

  /* describe PlyFace properties (just list of PlyVertex indices) */
  int ntriangles = m_triangles.count()/3;
  describe_element_ply ( ply, plyStrings[11], ntriangles );
  describe_property_ply ( ply, &face_props[0] );

  header_complete_ply ( ply );


  /* set up and write the PlyVertex elements */
  put_element_setup_ply ( ply, plyStrings[10] );

  for(int i=0; i<m_vertices.count(); i++)
    {
      myVertex vertex;
      vertex.x = m_vertices[i].x*m_scale.x;
      vertex.y = m_vertices[i].y*m_scale.y;
      vertex.z = m_vertices[i].z*m_scale.z;
      if (has_normals)
	{
	  vertex.nx = m_normals[i].x;
	  vertex.ny = m_normals[i].y;
	  vertex.nz = m_normals[i].z;
	}
      if (per_vertex_color)
	{
	  vertex.r = 255*m_vcolor[i].x;
	  vertex.g = 255*m_vcolor[i].y;
	  vertex.b = 255*m_vcolor[i].z;
	}
      put_element_ply ( ply, ( void * ) &vertex );
    }

  put_element_setup_ply ( ply, plyStrings[11] );
  face.nverts = 3 ;
  face.verts  = verts ;
  for(int i=0; i<m_triangles.count()/3; i++)
    {
      int v0 = m_triangles[3*i];
      int v1 = m_triangles[3*i+1];
      int v2 = m_triangles[3*i+2];

      face.verts[0] = v0;
      face.verts[1] = v1;
      face.verts[2] = v2;

      put_element_ply ( ply, ( void * ) &face );
    }

  close_ply ( ply );
  free_ply ( ply );
  fclose( fp ) ;

  QMessageBox::information(0, "Save Mesh", "done");
}

bool
TrisetObject::loadAssimpModel(QString flnm)
{
  m_position = Vec(0,0,0);
  m_scale = Vec(1,1,1);

  Assimp::Importer importer;
  const aiScene* scene = importer.ReadFile( flnm.toLatin1().data(), 
					    aiProcess_Triangulate            |
					    aiProcess_GenSmoothNormals       |
					    aiProcess_JoinIdenticalVertices  |
					    aiProcess_SortByPType);

  if(!scene)
    {
      QMessageBox::information(0, "Error Importing Asset",
			       QString("Couldn't load model %1").arg(flnm));
      return false;
    }
  
  m_vertices.clear();
  m_vcolor.clear();
  m_normals.clear();
  m_triangles.clear();
  m_uv.clear();
  m_meshInfo.clear();
  m_material.clear();
  
  //QMessageBox::information(0, "", QString("%1").arg(scene->mNumMeshes));
  
  int nvert = 0;
  for(int i=0; i<scene->mNumMeshes; i++)
    {
      int vStart = m_vertices.count();
      int iStart = m_triangles.count();
      
      aiMesh* mesh = scene->mMeshes[i];
      bool hasVertexColors = mesh->HasVertexColors(0);
      bool hasUV = mesh->HasTextureCoords(0);

      for(int j=0; j<mesh->mNumVertices; j++)
	{	  
	  aiVector3D pos = mesh->mVertices[j];
	  m_vertices << Vec(pos.x, pos.y, pos.z);
      
	  aiVector3D normal = mesh->mNormals[j];
	  m_normals << Vec(normal.x, normal.y, normal.z);
	  
	  if (hasVertexColors)
	    m_vcolor << Vec(mesh->mColors[0][j].r,
			    mesh->mColors[0][j].g,
			    mesh->mColors[0][j].b);
	  else
	    m_vcolor << Vec(0.7,0.7,0.7);

	  if (hasUV)
	    {
	      m_uv << Vec(mesh->mTextureCoords[0][j].x,
			  mesh->mTextureCoords[0][j].y,
			  mesh->mTextureCoords[0][j].z);
	    }
	} // verticeas

      for(int j=0; j<mesh->mNumFaces; j++)
	{
	  const aiFace& face = mesh->mFaces[j];
	  for(int k=0; k<face.mNumIndices; k++)
	    m_triangles << nvert+face.mIndices[k];
	} // faces

      QPolygon poly;
      poly << QPoint(vStart, m_vertices.count());
      poly << QPoint(iStart, m_triangles.count());
      poly << QPoint(mesh->mMaterialIndex, 0);
      m_meshInfo << poly;
      
      nvert += mesh->mNumVertices;
    }
  
  //--------------
  if (scene->mNumMaterials > 0)
    {
      bool texFound = false;
      for(int m=0; m<scene->mNumMaterials; m++)
	{
	  aiMaterial *mat = scene->mMaterials[m];
	  aiString path;
	  QString diffuseTexFile;
	  if (mat->GetTexture(aiTextureType_DIFFUSE, 0, &path) == AI_SUCCESS)
	    {
	      diffuseTexFile = QFileInfo(QFileInfo(flnm).dir(),
					 QString(path.data)).absoluteFilePath();
	      //QMessageBox::information(0, "", diffuseTexFile);
	      //break;
	      if (!QFile::exists(diffuseTexFile))
		{
		  QString mesg;
		  if (diffuseTexFile.isEmpty())
		    mesg = QString("Diffuse texture not found for %1").arg(flnm);
		  else
		    mesg = "Cannot locate " + diffuseTexFile;
		  
		  QMessageBox::information(0, "Texture Error", mesg);

		  QString imgFile = QFileDialog::getOpenFileName(0,
								 QString("Load diffuse texture"),
								 QFileInfo(flnm).dir().path(),
								 "Image Files (*.png *.tif *.bmp *.jpg *.gif)");
		  
		  QFileInfo f(imgFile);

		  if (f.exists() == true)
		    diffuseTexFile = imgFile;
		  else
		    diffuseTexFile = QString();
		}	    
	      m_material << diffuseTexFile;
	      texFound = texFound || (!diffuseTexFile.isEmpty());
	    }
	  else
	    m_material << QString();
	}
      
      if (!texFound)
	{
	  if (m_uv.count() > 0)
	    {
	      QString mesg;
	      mesg = QString("Diffuse texture not found for %1").arg(flnm);
	      QMessageBox::information(0, "Texture Error", mesg);

	      QString imgFile = QFileDialog::getOpenFileName(0,
							     QString("Load diffuse texture"),
							     QFileInfo(flnm).dir().path(),
							     "Image Files (*.png *.tif *.bmp *.jpg *.gif)");
	      
	      QFileInfo f(imgFile);
	      
	      if (f.exists() == true)
		{
		  for (int m=0; m<m_material.count(); m++)
		    m_material[m] = imgFile;
		}
	      else
		m_uv.clear();
	    }	  
	}
	  
//      if (m_uv.count() > 0 &&
//	  !QFile::exists(m_diffuseTexFile))
//	{
//	  QString mesg;
//	  if (m_diffuseTexFile.isEmpty())
//	    mesg = "Diffuse texture not found.";
//	  else
//	    mesg = "Cannot locate " + m_diffuseTexFile;
//	  QMessageBox::information(0, "Texture Error", mesg);
//	  QString imgFile = QFileDialog::getOpenFileName(0,
//							 QString("Load diffuse texture"),
//							 QFileInfo(flnm).dir().path(),
//							 "Image Files (*.png *.tif *.bmp *.jpg *.gif)");
//	  
//	  QFileInfo f(imgFile);
//	  if (f.exists() == true)
//	    m_diffuseTexFile = imgFile;
//	  else
//	    m_uv.clear();
//	}
    }
  //--------------


//  //--------------
//  {
//    QString mesg;
//    for(int i=0; i<m_meshInfo.count(); i++)
//      {
//	QPolygon poly = m_meshInfo[i];
//	mesg += QString("V : %1 %2       I : %3 %4     M : %5\n").\
//	  arg(poly.point(0).x()).arg(poly.point(0).y()).
//	  arg(poly.point(1).x()).arg(poly.point(1).y()).
//	  arg(poly.point(2).x());
//      }
//    for(int i=0; i<m_material.count(); i++)
//      mesg += QString("\nM : %1").arg(m_material[i]);
//    
//    QMessageBox::information(0, "", mesg);
//  }
//  //--------------

  

  float minX, maxX;
  float minY, maxY;
  float minZ, maxZ;
  minX = maxX = m_vertices[0].x;
  minY = maxY = m_vertices[0].y;
  minZ = maxZ = m_vertices[0].z;
  for(int i=0; i<m_vertices.count(); i++)
    {
      minX = qMin(minX, (float)m_vertices[i].x);
      maxX = qMax(maxX, (float)m_vertices[i].x);
      minY = qMin(minY, (float)m_vertices[i].y);
      maxY = qMax(maxY, (float)m_vertices[i].y);
      minZ = qMin(minZ, (float)m_vertices[i].z);
      maxZ = qMax(maxZ, (float)m_vertices[i].z);
    }

  Vec bmin = Vec(minX, minY, minZ);
  Vec bmax = Vec(maxX, maxY, maxZ);

//  QMessageBox::information(0, "", QString("%1 : %2\n %3 %4 %5\n %6 %7 %8").\
//			   arg(m_vertices.count()).\
//			   arg(m_triangles.count()).\
//			   arg(bmin.x).arg(bmin.y).arg(bmin.z).\
//			   arg(bmax.x).arg(bmax.y).arg(bmax.z));
  
  minX = floor(minX);
  minY = floor(minY);
  minZ = floor(minZ);
  maxX = ceil(maxX);
  maxY = ceil(maxY);
  maxZ = ceil(maxZ);
  m_nZ = maxX-minX+1;
  m_nY = maxY-minY+1;
  m_nX = maxZ-minZ+1;
  
  m_enclosingBox[0] = Vec(bmin.x, bmin.y, bmin.z);
  m_enclosingBox[1] = Vec(bmax.x, bmin.y, bmin.z);
  m_enclosingBox[2] = Vec(bmax.x, bmax.y, bmin.z);
  m_enclosingBox[3] = Vec(bmin.x, bmax.y, bmin.z);
  m_enclosingBox[4] = Vec(bmin.x, bmin.y, bmax.z);
  m_enclosingBox[5] = Vec(bmax.x, bmin.y, bmax.z);
  m_enclosingBox[6] = Vec(bmax.x, bmax.y, bmax.z);
  m_enclosingBox[7] = Vec(bmin.x, bmax.y, bmax.z);

  m_centroid = (bmin + bmax)/2;

  m_position = Vec(0,0,0);
  m_scale = Vec(1,1,1);

  m_fileName = flnm;

  return true;
}
