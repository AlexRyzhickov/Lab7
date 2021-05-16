#include "glview.h"
#include "QDebug"
#include <QList>
#include <QGuiApplication>
#include <QMatrix4x4>
#include <QOpenGLShaderProgram>
#include <QScreen>
#include <QtMath>
#include <material.h>


static const char *vertexShaderSource = R"(
    #version 330
    layout (location = 0) in  highp vec4 posAttr;
    uniform highp mat4 matrix;
    void main() {
       gl_Position = matrix * posAttr;
    }
)";

static const char *geometryShaderSource = R"(
    #version 330
    layout (triangles) in;
    layout (triangle_strip, max_vertices = 3) out;
    in lowp vec4 col[3];
    uniform highp mat4 view_matrix;
    uniform highp mat4 proj_matrix;
    out vec3 fragment_normal;
    out vec3 fragment_position;

    vec4[3] positions_from_gl_in() {
        vec4 positions[3];
        for (int i = 0; i < 3; i++)
            positions[i] = gl_in[i].gl_Position;
        return positions;
    }

    void emit_vertex(vec4 position) {
        gl_Position = proj_matrix * view_matrix * position;
        EmitVertex();
    }

    vec3 calculate_normal(vec4 positions[3]) {
        vec3 a = vec3(positions[2]) - vec3(positions[1]);
        vec3 b = vec3(positions[0]) - vec3(positions[1]);
        return normalize(cross(a, b));
    }

    void main(){
        vec4 positions[3] = positions_from_gl_in();
        fragment_normal = calculate_normal(positions);
        for (int i = 0; i < 3; i++) {
            fragment_position = positions[i].xyz;
            emit_vertex(positions[i]);
        }
        EndPrimitive();
    }
)";

static const char *fragmentShaderSource = R"(
    #version 420
    uniform float intensity;
    uniform vec3 view_position;
    uniform vec3 lightColor;
    uniform vec3 diffuse_color;
    uniform vec3 ambient_light;
    uniform vec3 specular_color;
    uniform vec3 light_position;
    uniform float shininess;
    out vec4 color;
    in vec3 fragment_normal;
    in vec3 fragment_position;

    void main() {
        color = vec4(0.0);
        vec3 light_color = lightColor * intensity;

        float light_distance = length(light_position - fragment_position);
        float attenuation = 1.0 / (light_distance * light_distance);
        vec3 light = light_color * attenuation;

        vec3 to_light = normalize(light_position - fragment_position);
        vec3 normal = fragment_normal;

        vec3 ambient = diffuse_color * ambient_light ;

        float diffuse_strength = max(dot(normal, to_light), 0.0);
        vec3 diffuse = diffuse_strength * diffuse_color * light;

        vec3 view_direction = normalize(view_position - fragment_position);
        vec3 halfway_direction = normalize(to_light + view_direction);
        float specular_strength = pow(max(dot(normal, halfway_direction), 0.0), shininess);
        vec3 specular = specular_strength * specular_color * light;

        color = vec4(ambient + diffuse + specular, 1.0);
    }
)";

glView::glView(QWidget *parent)
{
  color = new QColor(255,255,255,255);
  connect(&mpTimer, SIGNAL(timeout()), this, SLOT(repaint()));
  mpTimer.start(33);
}

void glView::initializeGL()
{

    material = new Material();
    light_position = new QVector3D(0.8f,0.8f,0.8f);
    initializeOpenGLFunctions();

    m_program = new QOpenGLShaderProgram(this);
    m_program->addShaderFromSourceCode(QOpenGLShader::Vertex, vertexShaderSource);
    m_program->addShaderFromSourceCode(QOpenGLShader::Geometry, geometryShaderSource);
    m_program->addShaderFromSourceCode(QOpenGLShader::Fragment, fragmentShaderSource);
    m_program->link();

    m_posAttr = m_program->attributeLocation("posAttr");
    Q_ASSERT(m_posAttr != -1);
    m_matrixUniform = m_program->uniformLocation("matrix");
    Q_ASSERT(m_matrixUniform != -1);
    v_matrixUniform = m_program->uniformLocation("view_matrix");
    Q_ASSERT(v_matrixUniform != -1);
    p_matrixUniform = m_program->uniformLocation("proj_matrix");
    Q_ASSERT(p_matrixUniform != -1);
    view_position = m_program->uniformLocation("view_position");
    Q_ASSERT(view_position != -1);
    diffuse_color = m_program->uniformLocation("diffuse_color");
    Q_ASSERT(diffuse_color != -1);
    ambient_light = m_program->uniformLocation("ambient_light");
    Q_ASSERT(ambient_light != -1);
    specular_color = m_program->uniformLocation("specular_color");
    Q_ASSERT(specular_color != -1);
    shininess = m_program->uniformLocation("shininess");
    Q_ASSERT(shininess != -1);
    light_positionUniform = m_program->uniformLocation("light_position");
    Q_ASSERT(light_positionUniform != -1);
    intensityUniform = m_program->uniformLocation("intensity");
    Q_ASSERT(intensityUniform != -1);
    lightColorUniform = m_program->uniformLocation("lightColor");
    Q_ASSERT(lightColorUniform != -1);
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
}

void glView::resizeGL(int w, int h)
{
  glViewport(0, 0, w, h);

  //mScaleFactorX = 800 / (float)w;
  //mScaleFactorY = 600 / (float)h;
}

void glView::paintGL()
{
    glEnable(GL_DEPTH_TEST);

    const qreal retinaScale = devicePixelRatio();
    glViewport(0, 0, width() * retinaScale, height() * retinaScale);

    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    m_program->bind();

    QMatrix4x4 proj_matrix;
    if(isPerspectiveOrOrtho){
        proj_matrix.perspective(60.0f, 4.0f / 3.0f, 0.1f, 3000.0f);
    }else{
        proj_matrix.ortho(-1.0f,1.0f,-1.0f,1.0f,0.1f,100.0f);
    }


    QMatrix4x4 view_matrix_eye;
    view_matrix_eye.rotate(camera_rotation_x, 1, 0, 0);
    view_matrix_eye.rotate(camera_rotation_y, 0, 1, 0);
    view_matrix_eye.rotate(camera_rotation_z, 0, 0, 1);
    QVector3D vector(0.8f + camera_translate_x,0.8f + camera_translate_y,0.8f + camera_translate_z);

    QVector3D eye = view_matrix_eye.map(vector);

    QVector3D up(0.0f,1.0f,0.0f);

    QVector3D center(0.0f,0.0f,0.0f);

    QMatrix4x4 view_matrix;
    view_matrix.lookAt(eye,center,up);

    QMatrix4x4 matrix;
    matrix.translate(translate_x, translate_y, translate_z);
    matrix.rotate(rotation_x, 1, 0, 0);
    matrix.rotate(rotation_y, 0, 1, 0);
    matrix.rotate(rotation_z, 0, 0, 1);

    matrix.scale(scale_x,scale_y,scale_z);
    m_program->setUniformValue(p_matrixUniform, proj_matrix);
    m_program->setUniformValue(v_matrixUniform, view_matrix);
    m_program->setUniformValue(m_matrixUniform, matrix);
    m_program->setUniformValue(intensityUniform, intensity);
    glUniform3f(lightColorUniform,color->red()/255,color->green()/255,color->blue()/255);
    glUniform3f(view_position,vector.x(),vector.y(),vector.z());
    glUniform3f(diffuse_color,material->diffuse_color.x(),material->diffuse_color.y(),material->diffuse_color.z());
    glUniform3f(ambient_light,material->ambient_light.x(),material->ambient_light.y(),material->ambient_light.z());
    glUniform3f(specular_color,material->specular_color.x(),material->specular_color.y(),material->specular_color.z());
    m_program->setUniformValue(shininess, material->shininess);
    glUniform3f(light_positionUniform,light_position->x(),light_position->y(),light_position->z());

    int countVertex = split_step;

    GLfloat vertices[countVertex * 2 * 3 * 3];

    QList<Vertex*> vertexes_low_face;
    QList<Vertex*> vertexes_high_face;
    QList<Triangle*> triangles;
    float r = radius_low_face;

    for (double degrees = 0; degrees < 360; degrees=degrees+360/countVertex) {
        double radians = qDegreesToRadians(degrees);
        vertexes_low_face.push_back(new Vertex(0.0f,y_low_face, r, radians, 1.0f, 1.0f, 1.0f,1.0f));
    }

    r = radius_high_face;

    double countCorner = countVertex;
    double offsetAngle = qDegreesToRadians((-1)*(90 - ((countCorner - 2) * 180 / countCorner)/2));
    float start_x = r * sin(offsetAngle);
    float start_z = r * cos(offsetAngle);

    for (double degrees = 0; degrees < 360; degrees=degrees+360/countVertex) {
        double radians = qDegreesToRadians(degrees);
        vertexes_high_face.push_back(new Vertex(start_x,y_high_face, start_z, radians, 1.0f, 1.0f, 1.0f,1.0f));
    }

    for (int i=0; i<countVertex; i++) {
       if (i+1==countVertex){
            triangles.push_back(new Triangle(vertexes_high_face.at(i),vertexes_low_face.at(0),vertexes_low_face.at(i)));
            triangles.push_back(new Triangle(vertexes_high_face.at(i),vertexes_high_face.at(0),vertexes_low_face.at(0)));
       }else{
            triangles.push_back(new Triangle(vertexes_high_face.at(i),vertexes_low_face.at(i+1),vertexes_low_face.at(i)));
            triangles.push_back(new Triangle(vertexes_high_face.at(i),vertexes_high_face.at(i+1),vertexes_low_face.at(i+1)));
       }
    }



    for (int i = 0; i < triangles.size(); i++) {
            vertices[i*9 + 0] = triangles.at(i)->getVertex_one()->getX();
            vertices[i*9 + 1] = triangles.at(i)->getVertex_one()->getY();
            vertices[i*9 + 2] = triangles.at(i)->getVertex_one()->getZ();

            vertices[i*9 + 3] = triangles.at(i)->getVertex_two()->getX();
            vertices[i*9 + 4] = triangles.at(i)->getVertex_two()->getY();
            vertices[i*9 + 5] = triangles.at(i)->getVertex_two()->getZ();

            vertices[i*9 + 6] = triangles.at(i)->getVertex_three()->getX();
            vertices[i*9 + 7] = triangles.at(i)->getVertex_three()->getY();
            vertices[i*9 + 8] = triangles.at(i)->getVertex_three()->getZ();           
    }


    glVertexAttribPointer(m_posAttr, 3, GL_FLOAT, GL_FALSE, 0, vertices);

    glEnableVertexAttribArray(m_posAttr);

    //glCullFace(GL_FRONT);
    if(isDrawFill){
        glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
    }else{
        glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
    }
    glDrawArrays(GL_TRIANGLES, 0, triangles.size() * 3);

    glDisableVertexAttribArray(m_colAttr);

    m_program->release();
    ++m_frame;


        QVector4D axis_x(1.0f,0.0f,0.0f,1.0f);
        QVector4D axis_y(0.0f,1.0f,0.0f,1.0f);
        QVector4D axis_z(0.0f,0.0f,1.0f,1.0f);

        QVector4D new_axis_x = view_matrix.map(axis_x);
        QVector4D new_axis_y = view_matrix.map(axis_y);
        QVector4D new_axis_z = view_matrix.map(axis_z);

        glBegin(GL_LINES);
        glColor3f (1.0, 1.0, 0.0);
        glVertex3f(0.0, 0.0, 0.0);
        glVertex3f(new_axis_x.x(), new_axis_x.y(), new_axis_x.z());

        glColor3f (0.0, 1.0, 0.0);
        glVertex3f(0.0, 0.0, 0.0);
        glVertex3f(new_axis_y.x(), new_axis_y.y(), new_axis_y.z());

        glColor3f (0.0, 0.0, 1.0);
        glVertex3f(0.0, 0.0, 0.0);
        glVertex3f(new_axis_z.x(), new_axis_z.y(), new_axis_z.z());

        glEnd();

        QMatrix4x4 mul_matrixes = proj_matrix * view_matrix;
        QVector4D lightPositionSource1 = mul_matrixes.map(QVector4D(light_position->x(),light_position->y(),light_position->z(),1.0f));
        glPointSize(10);
        glBegin(GL_POINTS);
        glColor3f (color->red()/255, color->green()/255, color->blue()/255);
        glVertex3f(lightPositionSource1.x(), lightPositionSource1.y(), lightPositionSource1.z());
        glEnd();


}



