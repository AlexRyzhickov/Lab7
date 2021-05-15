#ifndef GLVIEW_H
#define GLVIEW_H

#include <QGLWidget>
#include <QMouseEvent>
#include <QTimer>
#include <vertex.h>
#include <QList>
#include <QOpenGLShaderProgram>
#include <QOpenGLFunctions>
#include <triangle.h>

QT_BEGIN_NAMESPACE
class QPainter;
class QOpenGLContext;
class QOpenGLPaintDevice;
QT_END_NAMESPACE

struct glView : QGLWidget, protected QOpenGLFunctions
{
public:
  glView(QWidget *parent);
  void initializeGL() override;
  void resizeGL(int w, int h)override;
  void paintGL()override;
private:
   void initShaders();
private:
  QTimer mpTimer;

  GLint m_posAttr = 0;
  GLint m_colAttr = 0;
  GLint m_matrixUniform = 0;
  GLint v_matrixUniform = 0;
  GLint p_matrixUniform = 0;
  GLint intensityUniform = 0;
  GLint lightColorUniform = 0;
  GLint view_position = 0;
  QOpenGLShaderProgram *m_program = nullptr;
  int m_frame = 0;
public:
  int split_step = 3;
  float radius_high_face = 0.4f;
  float radius_low_face = 0.9f;
  float y_high_face = 0.4f;
  float y_low_face = -0.4f;
  float rotation_x = 0.0f;
  float rotation_y = 0.0f;
  float rotation_z = 0.0f;
  float translate_x = 0.0f;
  float translate_y = 0.0f;
  float translate_z = 0.0f;
  float scale_x = 1.0f;
  float scale_y = 1.0f;
  float scale_z = 1.0f;
  float camera_rotation_x = 0.0f;
  float camera_rotation_y = 0.0f;
  float camera_rotation_z = 0.0f;
  float camera_translate_x = 0.0f;
  float camera_translate_y = 0.0f;
  float camera_translate_z = 0.0f;
  bool isPerspectiveOrOrtho = true;
  bool isDrawFill = false;
  float intensity = 0;
  QColor *color = nullptr;
public slots:


};

#endif // GLVIEW_H
