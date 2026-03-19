#include <QtCore/QJsonValue>
#include <QtCore/QJsonArray>
#include <QtCore/QJsonObject>

// Compatibility shim for environments where headers reference
// QJsonValueConstRef::concrete(QJsonValueConstRef) but the linked QtCore
// binary does not export that symbol.
//
// Marked weak so it won't conflict on toolchains/environments where QtCore
// already provides a strong implementation.
#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0) && QT_VERSION < QT_VERSION_CHECK(7, 0, 0)
#if defined(__GNUC__) || defined(__clang__)
#define DW_WEAK_SYMBOL __attribute__((weak))
#else
#define DW_WEAK_SYMBOL
#endif

QT_BEGIN_NAMESPACE
DW_WEAK_SYMBOL QJsonValue QJsonValueConstRef::concrete(QJsonValueConstRef self) noexcept
{
    switch (concreteType(self)) {
    case QJsonValue::Null:
        return QJsonValue(QJsonValue::Null);
    case QJsonValue::Bool:
        return QJsonValue(concreteBool(self, false));
    case QJsonValue::Double:
        return QJsonValue(concreteDouble(self, 0.0));
    case QJsonValue::String:
        return QJsonValue(concreteString(self, QString()));
    case QJsonValue::Array:
        return QJsonValue(self.toArray());
    case QJsonValue::Object:
        return QJsonValue(self.toObject());
    case QJsonValue::Undefined:
        return QJsonValue(QJsonValue::Undefined);
    }
    return QJsonValue(QJsonValue::Undefined);
}
QT_END_NAMESPACE

#undef DW_WEAK_SYMBOL
#endif
