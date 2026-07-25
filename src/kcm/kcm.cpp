#include <KPluginFactory>
#include <KQuickConfigModule>

class KHeadlessKcm final : public KQuickConfigModule
{
    Q_OBJECT
public:
    KHeadlessKcm(QObject *parent, const KPluginMetaData &metadata)
        : KQuickConfigModule(parent, metadata)
    {
    }
};

K_PLUGIN_CLASS_WITH_JSON(KHeadlessKcm, "kcm_kheadless.json")

#include "kcm.moc"

