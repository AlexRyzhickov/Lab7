#include "glview.h"
#include "QDebug"
#include <QList>
#include <QGuiApplication>
#include <QMatrix4x4>
#include <QOpenGLShaderProgram>
#include <QScreen>
#include <QtMath>


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
//    uniform float intensity;
    uniform vec3 view_position;
    uniform vec4 lightColor;
    out vec4 color;
    in vec3 fragment_normal;
    in vec3 fragment_position;

    void apply_light(vec3 light_pos) {
        vec3 to_light = normalize(fragment_position - light_pos);
        color += vec4(0.5,0.5,0.8,1.0)* vec4(max(dot(to_light, fragment_normal), 0.0));
    }


    void main() {
        color = vec4(0.0);

        vec3 light_position = vec3(0.8,0.8,0.8);
        //vec3 view_position = vec3(0.8,0.8,0.8);


        float light_distance = length(light_position - fragment_position);
        float attenuation = 1.0 / (light_distance * light_distance);
        vec3 light = vec3(1.0,1.0,1.0) * attenuation;

        vec3 to_light = normalize(light_position - fragment_position);
        vec3 normal = fragment_normal;

        vec3 ambient = vec3(0.07568, 0.61424, 0.07568) * vec3(0.0215,0.1745,	0.0215);

        float diffuse_strength = max(dot(normal, to_light), 0.0);
        vec3 diffuse = diffuse_strength * vec3(0.07568,	0.61424, 0.07568) * light;

        vec3 view_direction = normalize(view_position - fragment_position);
        vec3 halfway_direction = normalize(to_light + view_direction);
        float specular_strength = pow(max(dot(normal, halfway_direction), 0.0), 0.6);
        vec3 specular = specular_strength * vec3(0.633,	0.727811, 0.633) * light;

        color = vec4(ambient + diffuse + specular, 1.0);




//        vec3 light_pos = vec3(0.8);

//        vec4 lightPosition = vec4(0.8,0.8,0.8, 1.0);
//        vec4 eyePosition = vec4(0.8,0.8,0.8, 1.0);
//        vec3 eyeVect = normalize(fragment_position.xyz - eyePosition.xyz);
//        vec3 lightVect = normalize(fragment_position.xyz - lightPosition.xyz);
//        vec3 reflectLight = normalize(reflect(lightVect,fragment_normal));
//        float len = length(fragment_position.xyz - eyePosition.xyz);
//        float specularFactor = 10.0;
//        float ambientFactor = 0.1;

//        vec4 diffColor = vec4(1.0,1.0,1.0,1.0) * 1 * max(0.0,dot(fragment_normal, -lightVect))
//        / (1.0 + 0.25 * pow(len,2));
//        color += diffColor;
//        vec4 ambientColor = ambientFactor * vec4(1.0,1.0,1.0,1.0);
//        color += ambientColor;
//        vec4 specularColor = vec4(1.0,1.0,1.0,1.0) * 1 * pow(max(0.0, dot(reflectLight, -eyeVect)),specularFactor)
//        / (1.0 + 0.25 * pow(len,2));;
//        color += specularColor;


//        vec3 to_light = normalize(fragment_position - light_pos);


//        apply_light(vec3(0.8));
//        apply_light(vec3(-0.4, 0.8, 0.8));
//        apply_light(vec3(0.0, 0.9, 0.0));

//        apply_light(vec3(-0.9, 1.2, -1.2));
//        apply_light(vec3(-0.9, -0.2, -1.2));
//        apply_light(vec3(-0.9, -0.2, 1.2));
    }
)";

glView::glView(QWidget *parent)
{
  color = new QColor(255,255,0,255);
  connect(&mpTimer, SIGNAL(timeout()), this, SLOT(repaint()));
  mpTimer.start(33);
}

void glView::initializeGL()
{

    initializeOpenGLFunctions();

    m_program = new QOpenGLShaderProgram(this);
    m_program->addShaderFromSourceCode(QOpenGLShader::Vertex, vertexShaderSource);
    m_program->addShaderFromSourceCode(QOpenGLShader::Geometry, geometryShaderSource);
    m_program->addShaderFromSourceCode(QOpenGLShader::Fragment, fragmentShaderSource);
    m_program->link();

//    if(!m_program->addShaderFromSourceFile(QOpenGLShader::Vertex,":/vshader.vsh"))
//        close();
//    if(!m_program->addShaderFromSourceFile(QOpenGLShader::Fragment,":/fshader.fsh"))
//        close();
//    if(!m_program->link())
//        close();

    m_posAttr = m_program->attributeLocation("posAttr");
    Q_ASSERT(m_posAttr != -1);
    //m_colAttr = m_program->attributeLocation("colAttr");
    //Q_ASSERT(m_colAttr != -1);
    m_matrixUniform = m_program->uniformLocation("matrix");
    Q_ASSERT(m_matrixUniform != -1);
    v_matrixUniform = m_program->uniformLocation("view_matrix");
    Q_ASSERT(v_matrixUniform != -1);
    p_matrixUniform = m_program->uniformLocation("proj_matrix");
    Q_ASSERT(p_matrixUniform != -1);
    view_position = m_program->uniformLocation("view_position");
    Q_ASSERT(view_position != -1);
//    intensityUniform = m_program->uniformLocation("intensity");
//    Q_ASSERT(intensityUniform != -1);
//    lightColorUniform = m_program->uniformLocation("lightColor");
//    Q_ASSERT(lightColorUniform != -1);
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
    view_matrix_eye.translate(camera_translate_x, camera_translate_y, camera_translate_z);


    QVector3D vector(0.8f,0.8f,0.8f);

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
    glUniform4f(lightColorUniform,color->red(),color->green(),color->blue(),color->alpha());
    glUniform3f(view_position,vector.x(),vector.y(),vector.z());

    int countVertex = split_step;

    GLfloat vertices[countVertex * 2 * 3 * 3];
    GLfloat colors[countVertex * 2 * 3 * 3];

    QList<Vertex*> list;
    QList<Vertex*> list2;
    QList<Triangle*> list3;
    float r = radius_low_face;

    int count = 0;
    for (double degrees = 0; degrees < 360; degrees=degrees+360/countVertex) {
        double radians = qDegreesToRadians(degrees);
        if(count==0){
            list.push_back(new Vertex(0.0f,y_low_face, r, radians, 1.0f, 0.0f, 0.0f,1.0f));
        }
        if(count==1){
            list.push_back(new Vertex(0.0f,y_low_face, r, radians, 0.0f, 1.0f, 0.0f,1.0f));
        }
        if(count==2){
            list.push_back(new Vertex(0.0f,y_low_face, r, radians, 0.0f, 0.0f, 1.0f,1.0f));
        }
        count++;
        if(count==3){
            count=0;
        }
    }

    r = radius_high_face;

    double countCorner = countVertex;
    double offsetAngle = qDegreesToRadians((-1)*(90 - ((countCorner - 2) * 180 / countCorner)/2));
    float start_x = r * sin(offsetAngle);
    float start_z = r * cos(offsetAngle);

    count = 0;
    for (double degrees = 0; degrees < 360; degrees=degrees+360/countVertex) {
        double radians = qDegreesToRadians(degrees);
        //list2.push_back(new Vertex(start_x,0.4f, start_z, radians, 1.0f, 0.0f, 0.0f,1.0f));
        if(count==0){
            list2.push_back(new Vertex(start_x,y_high_face, start_z, radians, 0.7f, 0.4f, 0.0f,1.0f));
        }
        if(count==1){
            list2.push_back(new Vertex(start_x,y_high_face, start_z, radians, 0.4f, 1.0f, 0.0f,1.0f));
        }
        if(count==2){
            list2.push_back(new Vertex(start_x,y_high_face, start_z, radians, 0.0f, 7.0f, 1.0f,1.0f));
        }
        count++;
        if(count==3){
            count=0;
        }
    }

    for (int i=0; i<countVertex; i++) {
       if (i+1==countVertex){
            list3.push_back(new Triangle(list2.at(i),list.at(0),list.at(i)));
            list3.push_back(new Triangle(list2.at(i),list2.at(0),list.at(0)));
       }else{
            list3.push_back(new Triangle(list2.at(i),list.at(i+1),list.at(i)));
            list3.push_back(new Triangle(list2.at(i),list2.at(i+1),list.at(i+1)));
       }
    }



    for (int i = 0; i < list3.size(); i++) {
            vertices[i*9 + 0] = list3.at(i)->getVertex_one()->getX();
            vertices[i*9 + 1] = list3.at(i)->getVertex_one()->getY();
            vertices[i*9 + 2] = list3.at(i)->getVertex_one()->getZ();

            vertices[i*9 + 3] = list3.at(i)->getVertex_two()->getX();
            vertices[i*9 + 4] = list3.at(i)->getVertex_two()->getY();
            vertices[i*9 + 5] = list3.at(i)->getVertex_two()->getZ();

            vertices[i*9 + 6] = list3.at(i)->getVertex_three()->getX();
            vertices[i*9 + 7] = list3.at(i)->getVertex_three()->getY();
            vertices[i*9 + 8] = list3.at(i)->getVertex_three()->getZ();

            colors[i*9 + 0] = list3.at(i)->getVertex_one()->getRed();
            colors[i*9 + 1] = list3.at(i)->getVertex_one()->getGreen();
            colors[i*9 + 2] = list3.at(i)->getVertex_one()->getBlue();

            colors[i*9 + 3] = list3.at(i)->getVertex_two()->getRed();
            colors[i*9 + 4] = list3.at(i)->getVertex_two()->getGreen();
            colors[i*9 + 5] = list3.at(i)->getVertex_two()->getBlue();

            colors[i*9 + 6] = list3.at(i)->getVertex_three()->getRed();
            colors[i*9 + 7] = list3.at(i)->getVertex_three()->getGreen();
            colors[i*9 + 8] = list3.at(i)->getVertex_three()->getBlue();
    }


    glVertexAttribPointer(m_posAttr, 3, GL_FLOAT, GL_FALSE, 0, vertices);
    //glVertexAttribPointer(m_colAttr, 3, GL_FLOAT, GL_FALSE, 0, colors);

    glEnableVertexAttribArray(m_posAttr);
    //glEnableVertexAttribArray(m_colAttr);

    //glCullFace(GL_FRONT);
    if(isDrawFill){
        glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
    }else{
        glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
    }
    glDrawArrays(GL_TRIANGLES, 0, list3.size() * 3);

    glDisableVertexAttribArray(m_colAttr);
    //glDisableVertexAttribArray(m_posAttr);

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

//        vec3(-0.9, 1.2, -1.2));
//                apply_light(vec3(-0.9, -0.2, -1.2))

        //QVector4D lightPosition(0.8f,0.8f,0.8f,1.0f);
        QMatrix4x4 a = proj_matrix * view_matrix;
        QVector4D lightPositionSource1 = a.map(QVector4D(0.8f,0.8f,0.8f,1.0f));
        //QVector4D lightPositionSource2 = a.map(QVector4D(0.0f, 0.9f, 0.0f,1.0f));
        //QVector4D lightPositionSource3 = a.map(QVector4D(0.8f,0.8f,0.8f,1.0f));
        glPointSize(10);
        glBegin(GL_POINTS);
        glColor3f (1.0, 1.0, 1.0);
        glVertex3f(lightPositionSource1.x(), lightPositionSource1.y(), lightPositionSource1.z());
//        glColor3f (1.0, 1.0, 1.0);
//        glVertex3f(lightPositionSource2.x(), lightPositionSource2.y(), lightPositionSource2.z());
        glEnd();


}

void glView::initShaders()
{
    if(!m_program->addShaderFromSourceFile(QOpenGLShader::Vertex,":/vshader"))
        close();
    if(!m_program->addShaderFromSourceFile(QOpenGLShader::Fragment,":/fshader"))
        close();
    if(!m_program->link())
        close();

}



