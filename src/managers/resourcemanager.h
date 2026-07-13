#ifndef RESOURCEMANAGER_H
#define RESOURCEMANAGER_H

#include <QPixmap>
#include <QString>

class ResourceManager
{
public:
    static QPixmap pixmap(const QString &resourcePath);

private:
    ResourceManager() = delete;
};

#endif // RESOURCEMANAGER_H
