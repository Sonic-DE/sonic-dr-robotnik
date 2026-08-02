#include "memoryfence.h"
#include "memorypressure.h"

#include <QTimer>

void MemoryFence::surroundMe()
{
    m_size = Size::Spacious;
    QTimer::singleShot(0, this, &MemoryFence::loaded);
}

MemoryFence::Size MemoryFence::size() const
{
    return m_size;
}

bool MemoryFence::registerDBusTypes()
{
    return false;
}

void MemoryFence::getUnit()
{
}

std::optional<qulonglong> MemoryFence::freeRAM()
{
    return std::nullopt;
}

void MemoryFence::getMemory()
{
}

void MemoryFence::applyProperties(qulonglong, qulonglong)
{
}

MemoryPressure::~MemoryPressure() = default;

void MemoryPressure::reset()
{
    m_level = Level::Low;
}

MemoryPressure::Level MemoryPressure::level() const
{
    return m_level;
}

MemoryPressure::MemoryPressure(QObject* parent)
    : QObject(parent)
{
}

MemoryPressure* MemoryPressure::instance()
{
    static MemoryPressure instance;
    return &instance;
}
