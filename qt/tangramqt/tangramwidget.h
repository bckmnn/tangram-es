#ifndef TANGELGLWIDGET_H
#define TANGELGLWIDGET_H

#include <QOpenGLWidget>
#include <QFile>
#include <data/clientGeoJsonSource.h>
#include <QMouseEvent>
#include <QDateTime>
#include <QGestureEvent>
#include "platform.h"

class TangramWidget : public QOpenGLWidget
{
    Q_OBJECT

public:
    TangramWidget(QWidget *parent = Q_NULLPTR);
    virtual ~TangramWidget();

    static void startUrlRequest(const std::string &url, UrlCallback callback);
    static void cancelUrlRequest(const std::string &url);

    void grabGestures(const QList<Qt::GestureType> &gestures);

public slots:
    void handleCallback(UrlCallback callback);

protected:
    bool event(QEvent *e) Q_DECL_OVERRIDE;

    void initializeGL() Q_DECL_OVERRIDE;
    void paintGL() Q_DECL_OVERRIDE;
    void resizeGL(int width, int height) Q_DECL_OVERRIDE;

    void mousePressEvent(QMouseEvent *event) Q_DECL_OVERRIDE;
    void mouseMoveEvent(QMouseEvent *event) Q_DECL_OVERRIDE;
    void mouseReleaseEvent(QMouseEvent *event) Q_DECL_OVERRIDE;

private:
    bool renderRequestEvent();
    bool mouseWheelEvent(QWheelEvent *ev);
    bool gestureEvent(QGestureEvent *ev);
    void panTriggered(QPanGesture *gesture);
    void pinchTriggered(QPinchGesture *gesture);
    void swipeTriggered(QSwipeGesture *gesture);

    QFile m_sceneFile;
    std::shared_ptr<Tangram::ClientGeoJsonSource> data_source;

    int position;
    qreal horizontalOffset;
    qreal verticalOffset;
    qreal rotationAngle;
    qreal scaleFactor;
    qreal currentStepScaleFactor;

    QDateTime m_lastRendering;

    QPoint m_lastMousePos;
    QPointF m_lastMouseSpeed;
    ulong m_lastMouseEvent;
    bool m_panning;
};

#endif // TANGELGLWIDGET_H