#pragma once

#include "IConnector.h"
#include "SocketConnector.h"

#include <QDebug>
#include <QSharedPointer>

// 连接器类型（置于全局作用域）
enum class ConnectorType
{
    SOCKET = 0,
    USB = 1,
    TEST = 2
};

class ConnectorFactory
{
public:
    static QSharedPointer<IConnector> create(ConnectorType type = ConnectorType::SOCKET, QObject* parent = nullptr)
    {
        switch (type)
        {
            case ConnectorType::SOCKET:
                return QSharedPointer<IConnector>(new SocketConnector(parent));
            case ConnectorType::USB:
                qWarning() << "ConnectorFactory: USB connector not implemented, returning null";
                return QSharedPointer<IConnector>();
            case ConnectorType::TEST:
                qWarning() << "ConnectorFactory: TEST connector not implemented, returning null";
                return QSharedPointer<IConnector>();
            default:
                qWarning() << "ConnectorFactory: Unknown connector type, returning null";
                return QSharedPointer<IConnector>();
        }
    }
};
