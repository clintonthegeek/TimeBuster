#include "deltachange.h"
#include "cal.h"
#include <QDebug>

void DeltaChange::resolveItem(Cal* cal)
{
    if (!cal || itemId.isEmpty()) {
        qDebug() << "DeltaChange: Cannot resolve item - no cal or itemId";
        return;
    }
    for (const auto &i : cal->items()) {
        if (i->id() == itemId) {
            item = i;
            qDebug() << "DeltaChange: Resolved item" << itemId << "in" << cal->id();
            break;
        }
    }
    if (!item) {
        qDebug() << "DeltaChange: Item" << itemId << "not found in" << cal->id();
    }
}
