#ifndef CAICHUANGPACKAGE_H
#define CAICHUANGPACKAGE_H

#include "package.h"
#include "card.h"
#include "skill.h"
#include "standard.h"

class ShoucaiCard: public SkillCard{
    Q_OBJECT

public:
    Q_INVOKABLE ShoucaiCard();

    virtual bool targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const;
    virtual void onEffect(const CardEffectStruct &effect) const;
};

class RuguCard: public SkillCard{
    Q_OBJECT

public:
    Q_INVOKABLE RuguCard();

    virtual void use(Room *room, ServerPlayer *source, const QList<ServerPlayer *> &targets) const;
};

class YiyeCard: public SkillCard{
    Q_OBJECT

public:
    Q_INVOKABLE YiyeCard();

    virtual bool targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const;
    virtual void onEffect(const CardEffectStruct &effect) const;
};

class FanjiaCard: public SkillCard{
    Q_OBJECT

public:
    Q_INVOKABLE FanjiaCard();

    virtual bool targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const;
    virtual void onEffect(const CardEffectStruct &effect) const;
};

class HaixiuJink: public Jink{
    Q_OBJECT

public:
    Q_INVOKABLE HaixiuJink(Card::Suit suit, int number);
};

class ChenxingCard: public SkillCard{
    Q_OBJECT

public:
    Q_INVOKABLE ChenxingCard();

    virtual void onEffect(const CardEffectStruct &effect) const;
};

class JishenCard: public SkillCard{
    Q_OBJECT

public:
    Q_INVOKABLE JishenCard();

    virtual void use(Room *room, ServerPlayer *source, const QList<ServerPlayer *> &targets) const;
    virtual bool targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const;
};

class TiandaoCard: public SkillCard{
    Q_OBJECT

public:
    Q_INVOKABLE TiandaoCard();

    virtual void use(Room *room, ServerPlayer *source, const QList<ServerPlayer *> &targets) const;
};

class JiaojuanCard: public SkillCard{
    Q_OBJECT

public:
    Q_INVOKABLE JiaojuanCard();

    virtual bool targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const;
    virtual void onEffect(const CardEffectStruct &effect) const;
};

class CaichuangPackage:public Package{
    Q_OBJECT

public:
    CaichuangPackage();
    void addGenerals();
};

#endif // CAICHUANGPACKAGE_H
