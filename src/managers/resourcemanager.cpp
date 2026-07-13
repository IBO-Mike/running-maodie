#include "resourcemanager.h"

QPixmap ResourceManager::pixmap(const QString &resourcePath)
{
    return QPixmap(resourcePath);
}
