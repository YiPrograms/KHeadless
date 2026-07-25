#pragma once

#include <QDBusArgument>
#include <QList>
#include <QMetaType>
#include <QVariantList>
#include <QVariantMap>

namespace KHeadless
{

struct ObjectList {
    QList<QVariantMap> values;

    QVariantList toVariantList() const
    {
        QVariantList result;
        result.reserve(values.size());
        for (const auto &value : values) {
            result.append(value);
        }
        return result;
    }

    static ObjectList fromVariantList(const QVariantList &source)
    {
        ObjectList result;
        result.values.reserve(source.size());
        for (const auto &value : source) {
            result.values.append(value.toMap());
        }
        return result;
    }
};

inline QDBusArgument &operator<<(QDBusArgument &argument, const ObjectList &list)
{
    argument.beginArray(qMetaTypeId<QVariantMap>());
    for (const auto &value : list.values) {
        argument << value;
    }
    argument.endArray();
    return argument;
}

inline const QDBusArgument &operator>>(const QDBusArgument &argument, ObjectList &list)
{
    list.values.clear();
    argument.beginArray();
    while (!argument.atEnd()) {
        QVariantMap value;
        argument >> value;
        list.values.append(value);
    }
    argument.endArray();
    return argument;
}

}

Q_DECLARE_METATYPE(KHeadless::ObjectList)

