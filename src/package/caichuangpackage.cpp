#include "caichuangpackage.h"
#include "general.h"
#include "standard.h"
#include "skill.h"
#include "engine.h"
#include "client.h"
#include "clientplayer.h"
#include "carditem.h"
#include "serverplayer.h"
#include "room.h"
#include "standard-skillcards.h"
#include "joypackage.h"
#include "ai.h"
#include "maneuvering.h"

#include <QCommandLinkButton>
#include <QMessageBox>

//µ°ÌÛ
DantengCard::DantengCard(){

}

bool DantengCard::targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const{
    if(!targets.isEmpty())
        return false;

    return true;
}

void DantengCard::onEffect(const CardEffectStruct &effect) const{
    int x = qMin(5, effect.to->getLostHp());
    if(x <= 0)
        return;

    effect.to->drawCards(x);
}

class dantengViewAsSkill: public ZeroCardViewAsSkill{
public:
    dantengViewAsSkill():ZeroCardViewAsSkill("danteng"){

    }

    virtual bool isEnabledAtPlay(const Player *player) const{
        return false;
    }

    virtual bool isEnabledAtResponse(const Player *player, const QString &pattern) const{
        return pattern == "@@danteng";
    }

    virtual const Card *viewAs() const{
        return new DantengCard;
    }
};

class danteng: public MasochismSkill{
public:
    danteng():MasochismSkill("danteng"){
        view_as_skill = new dantengViewAsSkill;
    }

    virtual void onDamaged(ServerPlayer *shaolong, const DamageStruct &damage) const{
        Room *room = shaolong->getRoom();
        room->askForUseCard(shaolong, "@@danteng", "@danteng");
    }
};

//²¹µ°
class budan: public TriggerSkill{
public:
    budan():TriggerSkill("budan$"){
        events << HpLost << HpChanged << Damaged;
    }

    virtual bool triggerable(const ServerPlayer *target) const{
        return target->getKingdom() == "dan";
    }

    virtual bool trigger(TriggerEvent event, ServerPlayer *player, QVariant &) const{
        Room *room = player->getRoom();
        QList<ServerPlayer *> shaolongs;
        QList<ServerPlayer *> players = room->getOtherPlayers(player);
        foreach(ServerPlayer *p, players){
            if(p->hasLordSkill("budan")){
                shaolongs << p;
            }
        }

        if(event == HpLost) room->setPlayerFlag(player, "bulegedan");
        else if(player->isAlive() && event == Damaged){
        foreach(ServerPlayer *shaolong, shaolongs){
            QVariant who = QVariant::fromValue(shaolong);
            if(player->askForSkillInvoke(objectName(), who)){
                JudgeStruct judge;
                judge.pattern = QRegExp("(.*):(Heart):(.*)");
                judge.good = true;
                judge.reason = "budan";
                judge.who = player;

                room->judge(judge);

                if(judge.isGood()){
                    room->playSkillEffect(objectName());

                    RecoverStruct recover;
                    recover.who = player;
                    room->recover(shaolong, recover);
                }
            }
        }
    }else if(player->getHp() > 0 && event == HpChanged && player->hasFlag("bulegedan")){
        room->setPlayerFlag(player, "-bulegedan");
        foreach(ServerPlayer *shaolong, shaolongs){
            QVariant who = QVariant::fromValue(shaolong);
            if(player->askForSkillInvoke(objectName(), who)){
                JudgeStruct judge;
                judge.pattern = QRegExp("(.*):(Heart):(.*)");
                judge.good = true;
                judge.reason = "budan";
                judge.who = player;

                room->judge(judge);

                if(judge.isGood()){
                    room->playSkillEffect(objectName());

                    RecoverStruct recover;
                    recover.who = player;
                    room->recover(shaolong, recover);
                }
            }
        }
    }
    return false;
    }
};

//ºâÌì
class hengtian: public TriggerSkill{
public:
    hengtian():TriggerSkill("hengtian"){
        events << PhaseChange << CardLost << CardLostDone << HpChanged << AskForPeachesDone;

        frequency = Frequent;
    }

    virtual bool trigger(TriggerEvent , ServerPlayer *tianjie, QVariant &data) const{
        int num = tianjie->getHandcardNum();
        int num2 = tianjie->getHp();
        if(num < num2 && tianjie->getPhase() == Player::NotActive){

                    tianjie->drawCards(num2-num);
        }
        return false;
    }
};

//ÌìµÀ
TiandaoCard::TiandaoCard(){
    target_fixed = true;
}

void TiandaoCard::use(Room *room, ServerPlayer *tianjie, const QList<ServerPlayer *> &targets) const{

}

class TiandaoViewAsSkill:public OneCardViewAsSkill{
public:
    TiandaoViewAsSkill():OneCardViewAsSkill(""){

    }

    virtual bool isEnabledAtPlay(const Player *player) const{
        return false;
    }

    virtual bool isEnabledAtResponse(const Player *player, const QString &pattern) const{
        return  pattern == "@tiandao";
    }

    virtual bool viewFilter(const CardItem *to_select) const{
        if(to_select->isEquipped())
            return false;

        int value = Self->getMark("tiandao");
        if(value == 1)
            return to_select->getFilteredCard()->isBlack();
        else if(value == 2)
            return to_select->getFilteredCard()->isRed();

        return false;
    }

    virtual const Card *viewAs(CardItem *card_item) const{
        TiandaoCard *card = new TiandaoCard;
        card->addSubcard(card_item->getFilteredCard());

        return card;
    }
};

class Tiandao: public TriggerSkill{
public:
    Tiandao():TriggerSkill("tiandao"){
        view_as_skill = new TiandaoViewAsSkill;

        events << AskForRetrial;
    }

    virtual bool triggerable(const ServerPlayer *target) const{
        if(!TriggerSkill::triggerable(target))
            return false;

        else
            return true;
    }

    virtual bool trigger(TriggerEvent , ServerPlayer *player, QVariant &data) const{

        Room *room = player->getRoom();
        room->setPlayerMark(player, "tiandao", 0);
        JudgeStar judge = data.value<JudgeStar>();

        QStringList prompt_list;
        prompt_list << "@tiandao-card" << judge->who->objectName()
                << "" << judge->reason << judge->card->getEffectIdString();
        QString prompt = prompt_list.join(":");

        player->tag["Judge"] = data;
        room->setPlayerMark(player, "tiandao", judge->card->isRed() ? 1 : 2);
        const Card *card = room->askForCard(player, "@tiandao", prompt);

        if(card){
            player->obtainCard(judge->card);

            judge->card = Sanguosha->getCard(card->getEffectiveId());
            room->moveCardTo(judge->card, NULL, Player::Special);

            LogMessage log;
            log.type = "$ChangedJudge";
            log.from = player;
            log.to << judge->who;
            log.card_str = card->getEffectIdString();
            room->sendLog(log);

            room->sendJudgeResult(judge);

        }

        return false;
    }
};

//ºÚÓ°
class heiying:public TriggerSkill{
public:
    heiying():TriggerSkill("heiying"){
        frequency = Frequent;
        events << CardUsed << CardResponsed;
    }

    virtual bool trigger(TriggerEvent event, ServerPlayer *ziniang, QVariant &data) const{
        CardStar card = NULL;
        if(event == CardUsed){
            CardUseStruct use = data.value<CardUseStruct>();
            card = use.card;
        }else if(event == CardResponsed)
            card = data.value<CardStar>();

        if(card->isBlack() && (card->isNDTrick() || card->getTypeId() == Card::Basic)){
            Room *room = ziniang->getRoom();
            if(room->askForSkillInvoke(ziniang, objectName())){
                room->playSkillEffect(objectName());
                ziniang->drawCards(1);
            }
        }

        return false;
    }
};

//¿ªºÚ
KaiheiCard::KaiheiCard(){
    once = true;
}

bool KaiheiCard::targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const{
    return targets.isEmpty() && !to_select->isKongcheng();
}

void KaiheiCard::onEffect(const CardEffectStruct &effect) const{
    effect.from->getRoom()->doKaihei(effect.from, effect.to);
}

class kaihei: public ZeroCardViewAsSkill{
public:
    kaihei():ZeroCardViewAsSkill("kaihei"){
    }

    virtual bool isEnabledAtPlay(const Player *player) const{
        return !player->hasUsed("KaiheiCard");
    }

    virtual const Card *viewAs() const{
        return new KaiheiCard;
    }
};

//³ÕÏß
class chixian: public PhaseChangeSkill{
public:
    chixian():PhaseChangeSkill("chixian"){

    }

    virtual bool onPhaseChange(ServerPlayer *ziming) const{
        if(ziming->getPhase() == Player::Finish){
            Room *room = ziming->getRoom();
            if(room->askForSkillInvoke(ziming, objectName())){
                room->loseHp(ziming);
                room->playSkillEffect(objectName());
                if(ziming->getHp() > 0){
                    ziming->drawCards(3);
                    QList<int> yiji_cards = ziming->handCards().mid(ziming->getHandcardNum() - 3);

                    while(room->askForYiji(ziming, yiji_cards));
                }

            }
        }

        return false;
    }
};

//³¾ÐÇ
ChenxingCard::ChenxingCard(){
    once = true;
}

void ChenxingCard::onEffect(const CardEffectStruct &effect) const{
    int n = subcardsLength();
    Room *room = effect.from->getRoom();
    QString choice = room->askForChoice(effect.from, "chenxing", "cxq+cxm");
    if(choice == "cxq"){
        if(effect.to->getCardCount(true) > n)
        room->askForDiscard(effect.to, "chenxing", n, false, true);
        else{
            effect.to->throwAllEquips();
            effect.to->throwAllEquips();
        }
    }else if(choice == "cxm")
        effect.to->drawCards(n);
}

class chenxingViewAsSkill: public ViewAsSkill{
public:
    chenxingViewAsSkill():ViewAsSkill("chenxing"){

    }

    virtual bool viewFilter(const QList<CardItem *> &selected, const CardItem *to_select) const{
        return !to_select->isEquipped();
    }

    virtual bool isEnabledAtPlay(const Player *player) const{
        return player->getPhase() == Player::Finish && !player->hasUsed("ChenxingCard");
    }

    virtual bool isEnabledAtResponse(const Player *player, const QString &pattern) const{
        return  pattern.startsWith("@@chenxing");
    }

    virtual const Card *viewAs(const QList<CardItem *> &cards) const{
        if(cards.isEmpty())
            return NULL;

        ChenxingCard *card = new ChenxingCard;
        card->addSubcards(cards);
        return card;
    }
};

class chenxing: public PhaseChangeSkill{
public:
    chenxing():PhaseChangeSkill("chenxing"){
        view_as_skill = new chenxingViewAsSkill;
    }

    virtual bool onPhaseChange(ServerPlayer *yujian) const{
        Room *room = yujian->getRoom();

        if(yujian->getPhase() == Player::Discard && !yujian->isKongcheng()){
            if(room->askForUseCard(yujian, "@@chenxing", "@chenxing")){
                return false;
            }
        }
        return false;
    }
};

//ÒþÅÍ
class yinpan: public ProhibitSkill{
public:
    yinpan():ProhibitSkill("yinpan"){

    }

    virtual bool isProhibited(const Player *from, const Player *to, const Card *card) const{
        if(card->inherits("Slash") && from->getHp() > to->getHp())
            return true;
        else
            return false;
    }
};

//ÒÅ¾­
class yijing:public MasochismSkill{
public:
    yijing():MasochismSkill("yijing"){
    }

    virtual void onDamaged(ServerPlayer *hanshi, const DamageStruct &damage) const{
        Room *room = hanshi->getRoom();

        if(!room->askForSkillInvoke(hanshi, objectName()))
            return;

        room->playSkillEffect(objectName());
        QList<ServerPlayer *> players = room->getAlivePlayers();
         if(!players.isEmpty()){
                 ServerPlayer *target = room->askForPlayerChosen(hanshi, players, "yijing");
                 int card_id = room->askForCardChosen(hanshi, target, "hej", "yijing");
                 room->throwCard(card_id);
             }
     }
};

//ÑÖÉå
YansheCard::YansheCard(){
}

bool YansheCard::targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const{
    if(!targets.isEmpty())
        return false;

    return to_select->hasEquip();
}

void YansheCard::onEffect(const CardEffectStruct &effect) const{
    Room *room = effect.from->getRoom();
    room->loseHp(effect.from, 1);
    if(effect.from->isAlive()){
        room->playSkillEffect("yanshe");
        int card_id = room->askForCardChosen(effect.from, effect.to, "e", "yanshe");
        room->throwCard(card_id);
    }
}

class yansheViewAsSkill: public ZeroCardViewAsSkill{
public:
    yansheViewAsSkill():ZeroCardViewAsSkill("yanshe"){

    }

    virtual bool isEnabledAtPlay(const Player *player) const{
        return player->hasSkill("yanshe");
    }

    virtual bool isEnabledAtResponse(const Player *player, const QString &pattern) const{
        return false;
    }

    virtual const Card *viewAs() const{
        return new YansheCard;
    }
};

//Ê¬Ìå
class shiti: public TriggerSkill{
public:
    shiti():TriggerSkill("shiti"){
        events << Death;
        frequency = Compulsory;
    }

    virtual bool triggerable(const ServerPlayer *target) const{
        return target->hasSkill(objectName());
    }

    virtual bool trigger(TriggerEvent , ServerPlayer *player, QVariant &data) const{
        DamageStar damage = data.value<DamageStar>();
        ServerPlayer *killer = damage ? damage->from : NULL;
        if(killer){
            Room *room = player->getRoom();

            LogMessage log;
            log.type = "#dageshiti";
            log.from = player;
            log.to << killer;
            room->sendLog(log);

            killer->gainMark("shourendage", 1);
        }

        return false;
    }
};

//³ÁÄ¬
ChenmoCard::ChenmoCard(){
    once = true;
}

bool ChenmoCard::targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const{
    return targets.isEmpty();
}

void ChenmoCard::onEffect(const CardEffectStruct &effect) const{
    Room *room = effect.to->getRoom();
    JudgeStruct judge;
    judge.pattern = QRegExp("(.*):(heart):(.*)");
    judge.good = true;
    judge.reason = objectName();
    judge.who = effect.to;

    room->judge(judge);

    if(judge.isBad()){
        Room *room = effect.to->getRoom();
        LogMessage log;
        log.type = "$Chenmosuccess";
        log.from = effect.from;
        log.to << effect.to;
        room->sendLog(log);
        effect.to->tag["chenmo"] = effect.to->getGeneralName();
        QList<const Skill::Skill *> skills = effect.to->getVisibleSkillList();
        foreach(const Skill::Skill *skill, skills){
            if(skill->parent())
                room->detachSkillFromPlayer(effect.to, skill->objectName());
        }
        QString kingdom = effect.to->getKingdom();

        QString to_transfigure = effect.to->getGeneral()->isMale() ? "sujiang" : "sujiangf";
        room->setPlayerProperty(effect.to, "general", to_transfigure);
        room->setPlayerProperty(effect.to, "kingdom", kingdom);
        room->resetAI(effect.to);
    }else{
        LogMessage log;
        log.type = "$Chenmofail";
        log.from = effect.from;
        log.to << effect.to;
        room->sendLog(log);
    }
}

class chenmoViewAsSkill: public ZeroCardViewAsSkill{
public:
    chenmoViewAsSkill():ZeroCardViewAsSkill("chenmo"){
    }

    virtual const Card *viewAs() const{
        return new ChenmoCard;
    }

protected:
    virtual bool isEnabledAtPlay(const Player *player) const{
        return false;
    }

    virtual bool isEnabledAtResponse(const Player *player, const QString &pattern) const{
        return  pattern == "@@chenmo";
    }
};

class chenmo:public PhaseChangeSkill{
public:
    chenmo():PhaseChangeSkill("chenmo"){
        view_as_skill = new chenmoViewAsSkill;
    }

    virtual bool onPhaseChange(ServerPlayer *liying) const{
        Room *room = liying->getRoom();
        if(liying->getPhase() == Player::Judge)
            if(room->askForUseCard(liying, "@@chenmo", "@Chenmo-card"))
                room->playSkillEffect("chenmo");

        return false;
    }
};

class ChenmoStart: public GameStartSkill{
public:
    ChenmoStart():GameStartSkill("#chenmostart"){

    }

    virtual int getPriority() const{
        return -1;
    }

    virtual void onGameStart(ServerPlayer *player) const{
        Room *room = player->getRoom();
        QList<ServerPlayer *> players = room->getAllPlayers();
        foreach(ServerPlayer *player, players){
            player->tag["chenmo"] = "";
    }
    }
};

class ChenmoClear: public PhaseChangeSkill{
public:
    ChenmoClear():PhaseChangeSkill("#chenmo-clear"){

    }

    virtual bool triggerable(const ServerPlayer *target) const{
        return true;
    }

    virtual bool onPhaseChange(ServerPlayer *player) const{
        if(player->getPhase() == Player::Start && player->hasSkill("chenmo")){
            Room *room = player->getRoom();
            QList<ServerPlayer *> players = room->getAlivePlayers();
            foreach(ServerPlayer *player, players){
                if(player->tag["chenmo"] != ""){

                    QString kingdom = player->getKingdom();

                    room->setPlayerProperty(player, "general", player->tag["chenmo"]);
                    room->setPlayerProperty(player, "kingdom", kingdom);
                    QString newname = player->getGeneralName();
                    room->transfigure(player, newname, false, false);
                    /*QList<const Skill::Skill *> skills = player->getGeneral()->getVisibleSkillList();
                    foreach(const Skill::Skill *skill, skills){
                        room->acquireSkill(player, skill);
                        RoomSceneInstance->attachSkill(skill->objectName(), false);
                    }*/
                    room->resetAI(player);

                    player->tag["chenmo"] = "";

                    LogMessage log;
                    log.type = "#ChenmoClear";
                    log.from = player;
                    room->sendLog(log);
                }
            }
        }

        return false;
    }
};

class chenmodeadClear: public TriggerSkill{
public:
    chenmodeadClear():TriggerSkill("#chenmodead-clear"){
        events << Death;
    }

    virtual bool triggerable(const ServerPlayer *target) const{
        return target->hasSkill(objectName());
    }

    virtual bool trigger(TriggerEvent , ServerPlayer *player, QVariant &) const{
        Room *room = player->getRoom();
        QList<ServerPlayer *> players = room->getAlivePlayers();
        foreach(ServerPlayer *player, players){
            if(player->tag["chenmo"] != ""){

                QString kingdom = player->getKingdom();

                room->setPlayerProperty(player, "general", player->tag["chenmo"]);
                room->setPlayerProperty(player, "kingdom", kingdom);
                QString newname = player->getGeneralName();
                room->transfigure(player, newname, false, false);
                /*QList<const Skill::Skill *> skills = player->getGeneral()->getVisibleSkillList();
                foreach(const Skill::Skill *skill, skills){
                    room->attachSkillToPlayer(player, skill->objectName());
                }*/
                room->resetAI(player);

                player->tag["chenmo"] = "";

                LogMessage log;
                log.type = "#ChenmoClear";
                log.from = player;
                room->sendLog(log);
            }
        }

        return false;
    }
};

//ÉñÊ¥
class shensheng:public TriggerSkill{
public:
    shensheng():TriggerSkill("shensheng"){
        frequency = Compulsory;
        events << PhaseChange << SlashEffect << SlashHit << SlashMissed;
    }

    virtual bool trigger(TriggerEvent event, ServerPlayer *zhoushen, QVariant &data) const{
        SlashEffectStruct effect = data.value<SlashEffectStruct>();
            if(event == SlashEffect)
                effect.to->addMark("qinggang");
            return false;
        }
};

//Éñ»å
class shenhui:public TriggerSkill{
public:
    shenhui():TriggerSkill("shenhui"){
        frequency = Frequent;
        events << CardUsed << CardResponsed << FinishJudge;
    }
    virtual bool trigger(TriggerEvent , ServerPlayer *player, QVariant &data) const{
        player->getRoom()->playSkillEffect(objectName());
        return false;
    }
};

//¼ÀÉñ
JishenCard::JishenCard(){
    once = true;
}

void JishenCard::use(Room *room, ServerPlayer *, const QList<ServerPlayer *> &targets) const{
    ServerPlayer *zhoushen = targets.first();
    if(zhoushen->hasSkill("jishen")){
        zhoushen->obtainCard(this);
        room->setEmotion(zhoushen, "good");
    }
}

bool JishenCard::targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const{
    return targets.isEmpty() && to_select->hasLordSkill("jishen") && to_select != Self;
}

class jishenViewAsSkill: public OneCardViewAsSkill{
public:
    jishenViewAsSkill():OneCardViewAsSkill("jishenv"){

    }

    virtual bool isEnabledAtPlay(const Player *player) const{
        return !player->hasUsed("JishenCard") && player->getKingdom() == "shen";
    }

    virtual bool viewFilter(const CardItem *to_select) const{
        const Card *card = to_select->getCard();
        return card->getSuit() == Card::Heart;
    }

    virtual const Card *viewAs(CardItem *card_item) const{
        JishenCard *card = new JishenCard;
        card->addSubcard(card_item->getFilteredCard());

        return card;
    }
};

class jishen: public GameStartSkill{
public:
    jishen():GameStartSkill("jishen$"){

    }

    virtual void onGameStart(ServerPlayer *player) const{
        Room *room = player->getRoom();
        QList<ServerPlayer *> players = room->getAlivePlayers();
        foreach(ServerPlayer *player, players){
            if(player->getKingdom() == "shen" && !player->hasSkill("jishen"))
            room->attachSkillToPlayer(player, "jishenv");
        }
    }
};

//·çÁ÷
class fengliu: public DrawCardsSkill{
public:
    fengliu():DrawCardsSkill("fengliu"){

    }

    virtual int getDrawNum(ServerPlayer *player, int n) const{
        Room *room = player->getRoom();
        QList<ServerPlayer *> players = room->getOtherPlayers(player), targets;
         foreach(ServerPlayer *player, players){
             if(!player->getGeneral()->isMale() && player->isAlive())
                 targets << player;
     }

         if(!targets.isEmpty()){
             if(room->askForSkillInvoke(player, objectName())){
                 room->playSkillEffect(objectName());

                 ServerPlayer *target = room->askForPlayerChosen(player, targets, "fengliu");
                 target->drawCards(1);
                 target->turnOver();
            }else return n;

            player->setFlags(objectName());
            return n-1;
        }else
            return n;
    }
};

//²©Ñ§
class boxue: public OneCardViewAsSkill{
public:
    boxue():OneCardViewAsSkill("boxue"){

    }

    virtual bool viewFilter(const CardItem *to_select) const{
        return to_select->getFilteredCard()->getSuit() == Card::Heart;
    }

    virtual const Card *viewAs(CardItem *card_item) const{
        const Card *first = card_item->getCard();
        Snatch *snatch = new Snatch(first->getSuit(), first->getNumber());
        snatch->addSubcard(first->getId());
        snatch->setSkillName(objectName());
        return snatch;
    }
};

//°ÔÆø
class baqi: public DrawCardsSkill{
public:
    baqi():DrawCardsSkill("baqi"){

    }

    virtual int getDrawNum(ServerPlayer *xiaowen, int n) const{
        Room *room = xiaowen->getRoom();
        if(room->askForBaqi1(xiaowen, "baqi1"))
        {
            xiaowen->setFlags("Baqi1");
            n--;
        }
        if(room->askForBaqi2(xiaowen, "baqi2"))
        {
            xiaowen->setFlags("Baqi2");
            n--;
        }
            return n;
        }
};

//Éñ²ß
ShenceCard::ShenceCard(){
}

bool ShenceCard::targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const{
    if(to_select == Self)
        return false;

    if(targets.isEmpty())
        return true;

    if(targets.length() == 1){
        int max = Self->getLostHp();
        return max >= qAbs(to_select->getHp() - targets.first()->getHp());
    }

    return false;
}

bool ShenceCard::targetsFeasible(const QList<const Player *> &targets, const Player *Self) const{
    return targets.length() == 2;
}

void ShenceCard::use(Room *room, ServerPlayer *source, const QList<ServerPlayer *> &targets) const{
    ServerPlayer *a = targets.at(0);
    ServerPlayer *b = targets.at(1);

    int n1 = a->getHp();
    int n2 = b->getHp();

    // make sure a is front of b
    if(room->getFront(a, b) != a){
        qSwap(a, b);
        qSwap(n1, n2);
    }

    int blood = n1;

    a->setHp(n2);
    room->broadcastProperty(a, "hp");
    b->setHp(blood);
    room->broadcastProperty(b, "hp");

    LogMessage log;
    log.type = "#shence";
    log.from = a;
    log.to << b;
    log.arg = QString::number(n1);
    log.arg2 = QString::number(n2);
    room->sendLog(log);
}

class shenceViewAsSkill: public ZeroCardViewAsSkill{
public:
    shenceViewAsSkill():ZeroCardViewAsSkill("shence"){

    }

    virtual bool isEnabledAtPlay(const Player *player) const{
        return false;
    }

    virtual bool isEnabledAtResponse(const Player *player, const QString &pattern) const{
        return pattern == "@@shence";
    }

    virtual const Card *viewAs() const{
        return new ShenceCard;
    }
};

class shence: public MasochismSkill{
public:
    shence():MasochismSkill("shence"){
        view_as_skill = new shenceViewAsSkill;
    }

    virtual void onDamaged(ServerPlayer *dalao, const DamageStruct &damage) const{
        Room *room = dalao->getRoom();
        int x = damage.damage, i;
        for(i=0; i<x; i++){
            if(!room->askForUseCard(dalao, "@@shence", "@shence"))
                break;
        }
    }
};

//¹ûÖ­
GuozhiCard::GuozhiCard(){
    once = true;
    mute = true;
    will_throw = false;
}

bool GuozhiCard::targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const{
    if(!targets.isEmpty())
        return false;

    if(to_select->isKongcheng())
        return false;

    return true;
}

void GuozhiCard::use(Room *room, ServerPlayer *dalao, const QList<ServerPlayer *> &targets) const{
    ServerPlayer *target = targets.first();

    room->playSkillEffect("guozhi", 1);

    bool success = dalao->pindian(target, "guozhi", this);
    if(success){
        room->playSkillEffect("guozhi", 2);

        RecoverStruct recover;
        recover.who = dalao;
        room->recover(target, recover);
        RecoverStruct recover2;
        recover2.who = dalao;
        room->recover(dalao, recover2);

    }else{
        DamageStruct damage;
        damage.card = NULL;
        damage.from = target;
        damage.to = dalao;

        room->damage(damage);
    }
}
class guozhi: public OneCardViewAsSkill{
public:
    guozhi():OneCardViewAsSkill("guozhi"){

    }

    virtual bool isEnabledAtPlay(const Player *player) const{
        return ! player->hasUsed("GuozhiCard") && !player->isKongcheng();
    }

    virtual bool viewFilter(const CardItem *to_select) const{
        return !to_select->isEquipped();
    }

    virtual const Card *viewAs(CardItem *card_item) const{
        GuozhiCard *card = new GuozhiCard;
        card->addSubcard(card_item->getFilteredCard());
        return card;
    }
};

//Éñ¾Ô
ShenjuCard::ShenjuCard(){
    target_fixed = true;
}

void ShenjuCard::use(Room *room, ServerPlayer *source, const QList<ServerPlayer *> &) const{
    room->throwCard(this);
    if(source->isAlive()){
        int n = subcardsLength();
        source->gainMark("shenju", n);

        QString num = QString::number(n);

        LogMessage log;
        log.type = "#ShenjuDiscard";
        log.from = source;
        log.arg = num;
        room->sendLog(log);
    }
}

class shenju: public ViewAsSkill{
public:
    shenju():ViewAsSkill("shenju"){

    }

    virtual bool viewFilter(const QList<CardItem *> &selected, const CardItem *to_select) const{
        return true;
    }

    virtual bool isEnabledAtPlay(const Player *player) const{
        return true;
    }

    virtual const Card *viewAs(const QList<CardItem *> &cards) const{
        if(cards.isEmpty())
            return NULL;

        ShenjuCard *card = new ShenjuCard;
        card->addSubcards(cards);
        return card;
    }
};

class shenjuclear: public PhaseChangeSkill{
public:
    shenjuclear():PhaseChangeSkill("#shenjuclear"){

    }

    virtual bool onPhaseChange(ServerPlayer *wshen) const{
        if(wshen->getPhase() == Player::Finish && wshen->getMark("shenju") > 0){
            Room *room = wshen->getRoom();
                wshen->loseMark("shenju", 1000);
                LogMessage log;
                log.type = "#Shenjuclear";
                log.from = wshen;
                room->sendLog(log);
        }
        return false;
    }
};

//Ó°Ñ¹
class yingya: public TriggerSkill{
public:
    yingya():TriggerSkill("yingya"){

    }

    virtual bool trigger(TriggerEvent , ServerPlayer *wshen, QVariant &data) const{
        wshen->getRoom()->playSkillEffect(objectName());
        return false;
    }
};

//¾Ô»ê
class juhun: public TriggerSkill{
public:
    juhun():TriggerSkill("juhun"){
        events << Death;
        frequency = Limited;
    }

    virtual bool triggerable(const ServerPlayer *target) const{
        return target->hasSkill("juhun");
    }

    virtual bool trigger(TriggerEvent , ServerPlayer *player, QVariant &) const{
        Room *room = player->getRoom();
        QList<ServerPlayer *> players = room->getAlivePlayers();
        if(room->askForSkillInvoke(player, objectName())){
            ServerPlayer *target = room->askForPlayerChosen(player, players, objectName());
            room->broadcastInvoke("animate", "lightbox:$juhun");
            room->playSkillEffect(objectName());
            QString choice = room->askForChoice(target, "juhun", "feiying+mashu");
            if(choice == "mashu"){
                if(!player->hasSkill("mashu"))
                    room->acquireSkill(target, "mashu");
                LogMessage log;
                log.type = "#Junhunoma";
                log.from = player;
                log.to << target;
                room->sendLog(log);
            }else{
                if(!player->hasSkill("feiying"))
                    room->acquireSkill(target, "feiying");
                LogMessage log;
                log.type = "#Junhundma";
                log.from = player;
                log.to << target;
                room->sendLog(log);
            }

        }
        return false;
    }
};

//ÐÜ°Ô
class xiongba: public SlashBuffSkill{
public:
    xiongba():SlashBuffSkill("xiongba"){

    }

    virtual bool buff(const SlashEffectStruct &effect) const{
        ServerPlayer *xiongshen = effect.from;
        Room *room = xiongshen->getRoom();
        if(xiongshen->getPhase() != Player::Play)
            return false;

        int num = xiongshen->distanceTo(effect.to);
        if(xiongshen->getLostHp() >= num){
            if(xiongshen->askForSkillInvoke(objectName(), QVariant::fromValue(effect))){
                room->playSkillEffect(objectName());
                room->slashResult(effect, NULL);

                return true;
            }
        }

        return false;
    }
};
/*
class xiongba: public TriggerSkill{
public:
    xiongba():TriggerSkill("xiongba"){
        events << SlashMissed;
    }

    virtual bool trigger(TriggerEvent, ServerPlayer *player, QVariant &data) const{

    SlashEffectStruct effect = data.value<SlashEffectStruct>();

    Room *room = player->getRoom();
    if(player->askForSkillInvoke(objectName(), data))

        if(room->askForDiscard(player, "xiongba", 2, true, true)){
            room->playSkillEffect(objectName());
            room->slashResult(effect, NULL);
        }

    return false;

    }
};
*/

//ÐÇ³½
class xingchen: public TriggerSkill{
public:
    xingchen():TriggerSkill("xingchen"){
            events << Damaged;
            frequency = Frequent;

    }

    virtual bool triggerable(const ServerPlayer *target) const{
        return true;
    }

    virtual bool trigger(TriggerEvent , ServerPlayer *player, QVariant &data) const{
        Room *room = player->getRoom();
        ServerPlayer *sunchen = room->findPlayerBySkillName(objectName());
        if(sunchen && sunchen->askForSkillInvoke(objectName(), data)){
            QString pattern = QString("(.*):(spade|club):(.*)");
            /*int n = sunchen->aliveCount();

            QString pattern = QString("(.*):(spade|club):([%1-9]|..|J|Q|K)").arg(n);
            if(n==9)
            {
             pattern = QString("(.*):(spade|club):(..|J|Q|K)").arg(n);
            }

            if (n==10)
            {
              pattern = QString("(.*):(spade|club):(J|Q|K)").arg(n);
            }*/
            JudgeStruct judge;
            judge.pattern = QRegExp(pattern);
            judge.good = true;
            judge.reason = objectName();
            judge.who = sunchen;
            sunchen->setFlags("xingchen");

            room->judge(judge);
            if(!judge.isBad()){
            sunchen->obtainCard(judge.card);

            QList<int> xings = sunchen->handCards().mid(sunchen->getHandcardNum() - 1);

            foreach(int card_id, xings)
                sunchen->addToPile("xings", card_id, false);
        }
            sunchen->setFlags("-xingchen");

        }
        return false;
    }
};

class xingchenDraw: public PhaseChangeSkill{
public:
    xingchenDraw():PhaseChangeSkill("#xingchen-draw"){

    }

    virtual bool onPhaseChange(ServerPlayer *sunchen) const{

        if(sunchen->getPhase() == Player::Draw){
            QList<int> xings = sunchen->getPile("xings");
            if(xings.isEmpty())
                return false;

            Room *room = sunchen->getRoom();
            int n = 0;
            while(!xings.isEmpty()){
                int card_id = xings.first();
                if(card_id == -1)
                    break;
                n++;
                room->moveCardTo(Sanguosha->getCard(card_id), sunchen, Player::Hand, false);
                xings.removeOne(card_id);
            }

            LogMessage log;
            log.type = "#XingchenDraw";
            log.from = sunchen;
            log.arg = QString::number(n);
            room->sendLog(log);
        }

        return false;
    }
};

//Î×Êõ
class wushu: public OneCardViewAsSkill{
public:
    wushu():OneCardViewAsSkill("wushu"){

    }

    virtual bool isEnabledAtPlay(const Player *player) const{
        return false;
    }

    virtual bool isEnabledAtResponse(const Player *player, const QString &pattern) const{
        return  pattern.contains("peach") && player->getPhase() == Player::NotActive;
    }

    virtual bool viewFilter(const CardItem *to_select) const{
        const Card *card = to_select->getFilteredCard();
        int n = Self->aliveCount();
        return !card->isEquipped() && card->getNumber() % 2 == n % 2;
    }

    virtual const Card *viewAs(CardItem *card_item) const{
        const Card *first = card_item->getCard();
        Peach *peach = new Peach(first->getSuit(), first->getNumber());
        peach->addSubcard(first->getId());
        peach->setSkillName(objectName());
        return peach;
    }
};

//Ï×¾Õ
XianjuCard::XianjuCard(){
    once = true;
}

bool XianjuCard::targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const{
    if(targets.length() >= 1)
        return false;

    if(to_select == Self)
        return false;

    return to_select->isWounded() && to_select->getGeneral()->isMale();
}

void XianjuCard::onEffect(const CardEffectStruct &effect) const{

    Room *room = effect.from->getRoom();
    DamageStruct damage;
    damage.from = effect.to;
    damage.card = this;
    damage.to = effect.from;
    damage.nature = DamageStruct::Normal;
    room->damage(damage);

    RecoverStruct recover;
    recover.who = effect.from;
    room->recover(effect.to, recover);
    room->playSkillEffect(objectName());

    if(effect.from->isAlive()){

    QList<ServerPlayer *> targets = room->getOtherPlayers(effect.from);
    targets.removeOne(effect.to);
    if(!targets.isEmpty()){
       ServerPlayer *target = room->askForPlayerChosen(effect.from, targets, "xianju");
       room->cardEffect(new ThunderSlash(Card::NoSuit, 0), effect.from, target);
   }
   }
}

class xianju:public ZeroCardViewAsSkill{
public:
    xianju():ZeroCardViewAsSkill("xianju"){
    }

    virtual bool isEnabledAtPlay(const Player *player) const{
        return ! player->hasUsed("XianjuCard");
    }

    virtual const Card *viewAs() const{
        return new XianjuCard;
    }
};

//ÕýÌ«
ZhengtaiCard::ZhengtaiCard()
{
}


bool ZhengtaiCard::targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const{
    if(!targets.isEmpty())
        return false;

    if(to_select->hasFlag("source"))
        return false;

    if(!Self->canSlash(to_select))
        return false;

    int card_id = subcards.first();
    if(Self->getWeapon() && Self->getWeapon()->getId() == card_id)
        return Self->distanceTo(to_select) <= 1;
    else
        return true;
}

void ZhengtaiCard::onEffect(const CardEffectStruct &effect) const{
    effect.to->getRoom()->setPlayerFlag(effect.to, "zhengtai_target");
}

class zhengtaiViewAsSkill: public OneCardViewAsSkill{
public:
    zhengtaiViewAsSkill():OneCardViewAsSkill("zhengtai"){

    }

    virtual bool isEnabledAtPlay(const Player *player) const{
        return false;
    }

    virtual bool isEnabledAtResponse(const Player *player, const QString &pattern) const{
        return  pattern == "@zhengtai";
    }

    virtual bool viewFilter(const CardItem *to_select) const{
        return !to_select->isEquipped();
    }

    virtual const Card *viewAs(CardItem *card_item) const{
        ZhengtaiCard *zhengtai_card = new ZhengtaiCard;
        zhengtai_card->addSubcard(card_item->getFilteredCard());

        return zhengtai_card;
    }
};

class zhengtai: public TriggerSkill{
public:
    zhengtai():TriggerSkill("zhengtai"){
        view_as_skill = new zhengtaiViewAsSkill;
        events << CardEffected;
    }

    virtual bool triggerable(const ServerPlayer *target) const{
        return !target->isKongcheng();
    }

    virtual bool trigger(TriggerEvent , ServerPlayer *saobi, QVariant &data) const{
        CardEffectStruct effect = data.value<CardEffectStruct>();

        if(effect.card->inherits("Slash")){
            Room *room = saobi->getRoom();

            if(effect.to->hasSkill(objectName()) && effect.from){
                QList<ServerPlayer *> players = room->getOtherPlayers(saobi);
                players.removeOne(effect.from);

                bool can_invoke = false;
                foreach(ServerPlayer *player, players){
                    if(saobi->inMyAttackRange(player)){
                        can_invoke = true;
                        break;
                    }
                }

                if(can_invoke){
                    QString prompt = "@zhengtai:" + effect.from->objectName();
                    room->setPlayerFlag(effect.from, "source");
                    if(room->askForUseCard(saobi, "@@zhengtai", prompt)){
                        foreach(ServerPlayer *player, players){
                            if(player->hasFlag("zhengtai_target")){
                                room->setPlayerFlag(effect.from, "-source");
                                room->setPlayerFlag(player, "-zhengtai_target");
                                effect.to = player;

                                room->cardEffect(effect);
                                return true;
                            }
                        }
                    }
                    room->setPlayerFlag(effect.from, "-source");
                }
            }
        }else if(effect.multiple && effect.card->inherits("TrickCard") && effect.card->objectName() != "collateral"){
            Room *room = saobi->getRoom();

            if(effect.to->hasSkill(objectName()) && effect.from){
                QList<ServerPlayer *> players = room->getOtherPlayers(saobi);
                players.removeOne(effect.from);

                bool can_invoke = false;
                foreach(ServerPlayer *player, players){
                    if(saobi->inMyAttackRange(player)){
                        can_invoke = true;
                        break;
                    }
                }

                if(can_invoke){
                    QString prompt = QString("@zhengtai2:%1::%2").arg(effect.from->objectName()).arg(effect.card->objectName());
                    room->setPlayerFlag(effect.from, "source");
                    if(room->askForUseCard(saobi, "@@zhengtai", prompt)){
                        foreach(ServerPlayer *player, players){
                            if(player->hasFlag("zhengtai_target")){
                                room->setPlayerFlag(effect.from, "-source");
                                room->setPlayerFlag(player, "-zhengtai_target");
                                effect.to = player;

                                room->cardEffect(effect);
                                return true;
                            }
                        }
                    }
                    room->setPlayerFlag(effect.from, "-source");
                }
            }
        }

        return false;
    }
};

//»ùÓÑ
JiyouCard::JiyouCard(){
    target_fixed = true;
}

void JiyouCard::use(Room *room, ServerPlayer *saobi, const QList<ServerPlayer *> &targets) const{
    QList<ServerPlayer *> lieges = room->getLieges("ji", saobi);
    const Card *card = NULL;
    foreach(ServerPlayer *liege, lieges){
        QString result = room->askForChoice(liege, "jiyou", "accept+ignore");
        if(result == "ignore")
            continue;

        card = room->askForCard(liege, "analeptic", "@jiyou-analeptic");
        if(card){

            CardUseStruct card_use;
            card_use.card = card;
            card_use.from = saobi;

            room->useCard(card_use);
            return;
        }
    }
}

class jiyouViewAsSkill:public ZeroCardViewAsSkill{
public:
    jiyouViewAsSkill():ZeroCardViewAsSkill("jiyou$"){

    }

    virtual bool isEnabledAtPlay(const Player *player) const{
        return player->hasLordSkill("jiyou") && Analeptic::IsAvailable(player);
    }

    virtual const Card *viewAs() const{
        return new JiyouCard;
    }
};

class jiyou: public TriggerSkill{
public:
    jiyou():TriggerSkill("jiyou$"){
        events << CardAsked;
        default_choice = "ignore";

        view_as_skill = new jiyouViewAsSkill;
    }

    virtual bool triggerable(const ServerPlayer *target) const{
        return target->hasLordSkill("jiyou");
    }

    virtual bool trigger(TriggerEvent event, ServerPlayer *saobi, QVariant &data) const{
        QString pattern = data.toString();
        if(!pattern.contains("analeptic")){
            return false;}

        Room *room = saobi->getRoom();
        QList<ServerPlayer *> lieges = room->getLieges("ji", saobi);

        if(!room->askForSkillInvoke(saobi, objectName()))
            return false;

        room->playSkillEffect(objectName());
        foreach(ServerPlayer *liege, lieges){
            const Card *card = room->askForCard(liege, "analeptic", "@jiyou-analeptic:" + saobi->objectName());
            if(card){
                room->provide(card);
                return true;
            }
        }

        return false;
    }
};

class dyingjiyou: public TriggerSkill{
public:
    dyingjiyou():TriggerSkill("#dyingjiyou$"){
        events << AskForPeaches;
        default_choice = "ignore";
    }

    virtual bool triggerable(const ServerPlayer *target) const{
        return TriggerSkill::triggerable(target) && target->hasLordSkill("jiyou");
    }

    virtual bool trigger(TriggerEvent, ServerPlayer *saobi, QVariant &data) const{
        Room *room = saobi->getRoom();
            DyingStruct dying_data = data.value<DyingStruct>();
            if(dying_data.who != saobi)
                return false;

            if(saobi->askForSkillInvoke(objectName())){
            QList<ServerPlayer *> lieges = room->getLieges("ji", saobi);
            room->playSkillEffect(objectName());
            foreach(ServerPlayer *liege, lieges){
                const Card *card = room->askForCard(liege, "analeptic", "@jiyoudying-analeptic:" + saobi->objectName());
                 if(card){
                     CardUseStruct card_use;
                     card_use.card = card;
                     card_use.from = saobi;

                     room->useCard(card_use);
                     return true;
                }
            }
            }

        return false;
    }
};

class jiyouji:public ViewAsSkill{
public:
    jiyouji():ViewAsSkill("jiyouji"){
    }

    virtual bool isEnabledAtResponse(const Player *player, const QString &pattern) const{
        return pattern == "analeptic";
    }

    virtual bool isEnabledAtPlay(const Player *player) const{
        return Analeptic::IsAvailable(player);
    }

    virtual bool viewFilter(const QList<CardItem *> &selected, const CardItem *to_select) const{
        if(selected.isEmpty())
            return !to_select->isEquipped() && to_select->getFilteredCard()->getSuit() == Card::Spade;
        else if(selected.length() == 1){
            return !to_select->isEquipped() && to_select->getFilteredCard()->getSuit() == Card::Spade;
        }else
            return false;
    }

    virtual const Card *viewAs(const QList<CardItem *> &cards) const{
        if(cards.length() == 2){
            const Card *first = cards.first()->getCard();
            Analeptic *aa = new Analeptic(first->getSuit(), 0);
            aa->addSubcards(cards);
            aa->setSkillName(objectName());
            return aa;
        }else
            return NULL;
    }
};

class jiyoustart: public GameStartSkill{
public:
    jiyoustart():GameStartSkill("#jiyoustart$"){

    }

    virtual void onGameStart(ServerPlayer *player) const{
        Room *room = player->getRoom();
        QList<ServerPlayer *> players = room->getAlivePlayers();
        foreach(ServerPlayer *player, players){
            if(player->getKingdom() == "ji" && !player->hasSkill("jiyou"))
            room->attachSkillToPlayer(player, "jiyouji");
        }
    }
};

//±¬¾Õ
class baoju:public ViewAsSkill{
public:
    baoju():ViewAsSkill("baoju"){
    }

    virtual bool viewFilter(const QList<CardItem *> &selected, const CardItem *to_select) const{
        if(selected.length() <= 1)
            return !to_select->isEquipped() && to_select->getFilteredCard()->isBlack();
        else
            return false;
    }

    virtual const Card *viewAs(const QList<CardItem *> &cards) const{
        if(cards.length() == 2){
            const Card *first = cards.first()->getCard();
            SavageAssault *ss;
            if(first->getSuit() == Card::Heart)
                ss = new SavageAssault(Card::Spade, 0);
            else
                ss = new SavageAssault(first->getSuit(), 0);
            ss->addSubcards(cards);
            ss->setSkillName(objectName());
            return ss;
        }else
            return NULL;
    }
};

//¸¹ºÚ
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QCommandLinkButton>

class fuhei: public FilterSkill{
public:
    fuhei():FilterSkill("fuhei"){

    }

    virtual bool viewFilter(const CardItem *to_select) const{
        return to_select->getCard()->getSuit() == Card::Heart;
    }

    virtual const Card *viewAs(CardItem *card_item) const{
        const Card *card = card_item->getCard();
        Card *new_card = Card::Clone(card);
        if(new_card) {
            new_card->setSuit(Card::Spade);
            new_card->setSkillName(objectName());
            return new_card;
        }else
            return card;
    }
};

class fuheiRetrial: public TriggerSkill{
public:
    fuheiRetrial():TriggerSkill("#fuhei-retrial"){
        frequency = Compulsory;

        events << FinishJudge;
    }

    virtual bool trigger(TriggerEvent , ServerPlayer *player, QVariant &data) const{
        JudgeStar judge = data.value<JudgeStar>();
        if(judge->card->getSuit() == Card::Heart){
            LogMessage log;
            log.type = "#fuheiJudge";
            log.from = player;

            Card *new_card = Card::Clone(judge->card);
            new_card->setSuit(Card::Spade);
            new_card->setSkillName("fuhei");
            judge->card = new_card;

            player->getRoom()->sendLog(log);
        }

        return false;
    }
};

//·¢É§
class fasao: public TriggerSkill{
public:
    fasao():TriggerSkill("fasao$"){
        events << AskForPeaches;
        default_choice = "ignore";
    }

    virtual bool triggerable(const ServerPlayer *target) const{
        return TriggerSkill::triggerable(target) && target->hasLordSkill("fasao");
    }

    virtual bool trigger(TriggerEvent, ServerPlayer *saohuo, QVariant &data) const{
        Room *room = saohuo->getRoom();
            DyingStruct dying_data = data.value<DyingStruct>();
            if(dying_data.who != saohuo)
                return false;

            if(saohuo->askForSkillInvoke(objectName())){
            QList<ServerPlayer *> lieges = room->getLieges("ji", saohuo);
            room->playSkillEffect(objectName());
            foreach(ServerPlayer *liege, lieges){
                ask:
                const Card *card = room->askForCard(liege, "slash", "@fasao-slash:" + saohuo->objectName());
                 if(card){
                     JudgeStruct judge;
                     judge.pattern = QRegExp("(.*):(spade):(.*)");
                     judge.good = true;
                     judge.who = saohuo;
                     judge.reason = objectName();

                     room->judge(judge);
                     if(judge.isGood()){
                         RecoverStruct recover;
                         recover.who = liege;
                         room->recover(saohuo, recover);
                         if(saohuo->getHp() > 0)
                             return true;
                         else
                             goto ask;
                     }else
                         goto ask;
                }
            }
            }

        return false;
    }
};

//Ó²°Ô
class yingba: public OneCardViewAsSkill{
public:
    yingba():OneCardViewAsSkill("yingba"){

    }

    virtual bool viewFilter(const CardItem *to_select) const{
        return to_select->getCard()->getTypeId() == Card::Basic;
    }

    virtual const Card *viewAs(CardItem *card_item) const{
        const Card *card = card_item->getFilteredCard();

        YingbaCard *yingbacard = new YingbaCard(card->getSuit(), card->getNumber());
        yingbacard->setSkillName(objectName());
        yingbacard->setCancelable(false);
        yingbacard->addSubcard(card);

        return yingbacard;
    }
};

class yingbaClear: public PhaseChangeSkill{
public:
    yingbaClear():PhaseChangeSkill("#yingba-clear"){

    }

    virtual bool triggerable(const ServerPlayer *target) const{
        return true;
    }

    virtual bool onPhaseChange(ServerPlayer *target) const{
        if(target->getPhase() == Player::NotActive){
            Room *room = target->getRoom();
            QList<ServerPlayer *> players = room->getAllPlayers();
            foreach(ServerPlayer *player, players){
                if(player->getMark("imba") > 0){

                    player->loseMark("imba", 13);

                    LogMessage log;
                    log.type = "#YingbaClear";
                    log.from = player;
                    room->sendLog(log);
                }
            }
        }

        return false;
    }
};

//ÇÚ·Ü
class qinfen: public PhaseChangeSkill{
public:
    qinfen():PhaseChangeSkill("qinfen"){

    }

    virtual bool onPhaseChange(ServerPlayer *eshen) const{
        if(eshen->isKongcheng() && eshen->getPhase() == Player::NotActive){
            Room *room = eshen->getRoom();
            if(room->askForSkillInvoke(eshen, objectName())){
                room->playSkillEffect(objectName());
                if(eshen->getHp() > 0){
                    ServerPlayer *player = room->getCurrent();
                    room->throwCard(eshen->getWeapon());
                    if(eshen->hasFlag("drank")){
                        LogMessage log;
                        log.type = "#UnsetDrankEndOfTurn";
                        log.from = player;
                        room->sendLog(log);
                        room->setPlayerFlag(eshen, "-drank");
                    }
                    room->setCurrent(eshen);
                    room->getThread()->trigger(TurnStart, eshen);
                    room->setCurrent(player);
                }

            }
        }

        return false;
    }
};
/*class qinfen: public TriggerSkill{
public:
    qinfen():TriggerSkill("qinfen"){
        events << CardLost << CardLostDone << AskForPeachesDone;
        frequency = Frequent;

    }

    virtual bool trigger(TriggerEvent , ServerPlayer *eshen, QVariant &data) const{
        Room *room = eshen->getRoom();
            if(eshen->isKongcheng() && eshen->isAlive() && room->askForSkillInvoke(eshen, objectName())){
                    ServerPlayer *player = room->getCurrent();
                    room->throwCard(eshen->getWeapon());
                    eshen->clearFlags();
                    eshen->clearHistory();
                    room->setCurrent(eshen);
                    room->getThread()->trigger(TurnStart, eshen);
                    room->setCurrent(player);
                return true;
            }
        return false;
    }
};*/

//¼¢¿Ê
class jike: public OneCardViewAsSkill{
public:
    jike():OneCardViewAsSkill("jike"){

    }

    virtual bool viewFilter(const CardItem *to_select) const{
        return (!to_select->getFilteredCard()->isEquipped() && !to_select->getFilteredCard()->inherits("Shit"));
    }

    virtual const Card *viewAs(CardItem *card_item) const{
        const Card *first = card_item->getFilteredCard();
        Shite *shite = new Shite(first->getSuit(), first->getNumber());
        shite->addSubcard(first);
        shite->setSkillName(objectName());
        return shite;
    }

};

//Ð°¶ñ
class xiee: public OneCardViewAsSkill{
public:
    xiee():OneCardViewAsSkill("xiee"){

    }

    virtual bool viewFilter(const CardItem *to_select) const{
        return to_select->getFilteredCard()->getSuit() == Card::Spade || to_select->getFilteredCard()->getSuit() == Card::Club;
    }

    virtual bool isEnabledAtPlay(const Player *player) const{
        return false;
    }

    virtual bool isEnabledAtResponse(const Player *player, const QString &pattern) const{
        return  pattern == "nullification";
    }

    virtual const Card *viewAs(CardItem *card_item) const{
        const Card *first = card_item->getFilteredCard();
        Card *ncard = new Nullification(first->getSuit(), first->getNumber());
        ncard->addSubcard(first);
        ncard->setSkillName("xiee");

        return ncard;
    }
};

//Òì²Å
class yicaii: public TriggerSkill{
public:
    yicaii():TriggerSkill("yicaii"){
        events << CardFinished;
    }

    virtual bool triggerable(const ServerPlayer *target) const{
        return target->hasSkill("yicaii");
    }

    virtual bool trigger(TriggerEvent event, ServerPlayer *guanzhang, QVariant &data) const{
        CardStar card = NULL;
        if(event == CardFinished){
            CardUseStruct card_use = data.value<CardUseStruct>();
            card = card_use.card;

        if(card == NULL || !card->isNDTrick())
            return false;

            Room *room = guanzhang->getRoom();

            if(!card_use.from->hasFlag("used") && room->askForSkillInvoke(card_use.from, "yicaii")){
                if(room->askForCard(card_use.from, ".H", "@Yicai-Card")){
                    card_use.from->setFlags("used");
                    const Card *card = Sanguosha->cloneCard(card_use.card->objectName(), card_use.card->getSuit(), card_use.card->getNumber());
                    CardUseStruct use = data.value<CardUseStruct>();
                    use.card = card;
                    use.from = card_use.from;
                    room->useCard(use);
                    return true;
                }
            }
            card_use.from->setFlags("-used");
        }
        return false;
    }
};

//Ð¥ÖË
class xiaozhii: public TriggerSkill{
public:
    xiaozhii():TriggerSkill("xiaozhii"){
        events << Damage;
    }

    virtual bool trigger(TriggerEvent , ServerPlayer *xiaozhi, QVariant &data) const{
        DamageStruct damage = data.value<DamageStruct>();

        if(damage.to->isAlive() && damage.card && damage.card->inherits("Slash")
           && damage.to != xiaozhi){
            Room *room = xiaozhi->getRoom();
            if(room->askForSkillInvoke(xiaozhi, objectName(), data)){
                room->playSkillEffect(objectName(), 1);
                    room->loseHp(xiaozhi);
                    room->loseMaxHp(damage.to, damage.damage);
            }
        }

        return false;
    }
};

//º¦Ðß
HaixiuJink::HaixiuJink(Card::Suit suit, int number)
    :Jink(suit, number)
{

}

class haixiu: public FilterSkill{
public:
    haixiu():FilterSkill("haixiu"){

    }

    virtual bool viewFilter(const CardItem *to_select) const{
        return to_select->getCard()->isRed() && to_select->getCard()->inherits("Slash");
    }

    virtual const Card *viewAs(CardItem *card_item) const{
        const Card *card = card_item->getCard();
        HaixiuJink *jink = new HaixiuJink(card->getSuit(), card->getNumber());
        jink->addSubcard(card_item->getCard()->getId());
        jink->setSkillName(objectName());

        return jink;
    }
};

//·µ¼Ò
FanjiaCard::FanjiaCard(){
    once = true;
}

bool FanjiaCard::targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const{
    return targets.isEmpty();
}

void FanjiaCard::onEffect(const CardEffectStruct &effect) const{
    Room *room = effect.from->getRoom();
    room->playSkillEffect("fanjia");
    room->swapSeat(effect.to, effect.from);
    effect.from->turnOver();
    room->setCurrent(effect.to);
}

class fanjiaViewAsSkill: public ZeroCardViewAsSkill{
public:
    fanjiaViewAsSkill():ZeroCardViewAsSkill("fanjia"){
    }

    virtual const Card *viewAs() const{
        return new FanjiaCard;
    }

protected:
    virtual bool isEnabledAtPlay(const Player *player) const{
        return false;
    }

    virtual bool isEnabledAtResponse(const Player *player, const QString &pattern) const{
        return  pattern == "@@fanjia";
    }
};

class fanjia:public PhaseChangeSkill{
public:
    fanjia():PhaseChangeSkill("fanjia"){
        view_as_skill = new fanjiaViewAsSkill;
    }

    virtual bool onPhaseChange(ServerPlayer *zhonghao) const{
        Room *room = zhonghao->getRoom();
        if(zhonghao->getPhase() == Player::Finish)
            room->askForUseCard(zhonghao, "@@fanjia", "@Fanjia-card");

        return false;
    }
};

//ÖÐÖ¸
class zhongzhi: public TriggerSkill{
public:
    zhongzhi():TriggerSkill("zhongzhi"){
        events << DrawNCards << PhaseChange;
        frequency = Compulsory;
    }

    int getpeople(const Player *player) const{
        int num = 0;
        QList<const Player *> players = player->parent()->findChildren<const Player *>();
        foreach(const Player *player, players)
            if(!player->isDead())
            num += 1;

        return floor(num/2);
    }

    virtual bool trigger(TriggerEvent event, ServerPlayer *zhonghao, QVariant &data) const{
        if(event == DrawNCards){
            int x = getpeople(zhonghao);
            data = data.toInt() + x;

            Room *room = zhonghao->getRoom();
            LogMessage log;
            log.type = "#ZhongzhiGood";
            log.from = zhonghao;
            log.arg = QString::number(x);
            room->sendLog(log);

            room->playSkillEffect("zhongzhi", x);

        }else if(event == PhaseChange && zhonghao->getPhase() == Player::Discard){
            int x = getpeople(zhonghao);
            int total = zhonghao->getEquips().length() + zhonghao->getHandcardNum();
            Room *room = zhonghao->getRoom();

            if(total <= x){
                zhonghao->throwAllHandCards();
                zhonghao->throwAllEquips();

                LogMessage log;
                log.type = "#ZhongzhiWorst";
                log.from = zhonghao;
                log.arg = QString::number(total);
                room->sendLog(log);

            }else{
                room->askForDiscard(zhonghao, "zhongzhi", x, false, true);

                LogMessage log;
                log.type = "#ZhongzhiBad";
                log.from = zhonghao;
                log.arg = QString::number(x);
                room->sendLog(log);
            }
        }

        return false;
    }
};

//ÊÕ²Æ
ShoucaiCard::ShoucaiCard(){
    once = true;
}

bool ShoucaiCard::targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const{
    if(targets.length() >= 2)
        return false;

    if(to_select == Self)
        return false;

    return !to_select->isNude();
}

void ShoucaiCard::onEffect(const CardEffectStruct &effect) const{

    Room *room = effect.from->getRoom();
    int card_id = room->askForCardChosen(effect.from, effect.to, "he", "shoucai");
    const Card *card = Sanguosha->getCard(card_id);
    bool is_public = room->getCardPlace(card_id) != Player::Hand;
    room->moveCardTo(card, effect.from, Player::Hand, is_public ? true : false);
    card = NULL;

    ask:
    card = room->askForCard(effect.from, ".", "@Shoucai-Card#");
    if(card != NULL)
    room->moveCardTo(card, effect.to, Player::Hand);
    else goto ask;
}

class shoucai:public ZeroCardViewAsSkill{
public:
    shoucai():ZeroCardViewAsSkill("shoucai"){
    }

    virtual bool isEnabledAtPlay(const Player *player) const{
        return ! player->hasUsed("ShoucaiCard");
    }

    virtual const Card *viewAs() const{
        return new ShoucaiCard;
    }
};

//¿É°®
class keaiClear: public TriggerSkill{
public:
    keaiClear():TriggerSkill("#keai-clear"){
        events << PhaseChange;
    }

    virtual bool triggerable(const ServerPlayer *target) const{
        return true;
    }

    virtual bool trigger(TriggerEvent, ServerPlayer *player, QVariant &) const{
        if(player->getPhase() == Player::NotActive)
            player->getRoom()->setTag("keai", QVariant());

        return false;
    }
};

class keai: public TriggerSkill{
public:
    keai():TriggerSkill("keai"){
        events << Damaged << Predamaged;

        frequency = Compulsory;
    }

    virtual bool trigger(TriggerEvent event, ServerPlayer *player, QVariant &data) const{
        Room *room = player->getRoom();

        if(player->getPhase() != Player::NotActive)
            return false;

        if(event == Damaged){
            room->setTag("keai", player->objectName());

            room->playSkillEffect(objectName());

            LogMessage log;
            log.type = "#keaiDamaged";
            log.from = player;
            room->sendLog(log);

        }else if(event == Predamaged){
            if(room->getTag("keai").toString() != player->objectName())
                return false;

            room->playSkillEffect(objectName());

            DamageStruct damage = data.value<DamageStruct>();
                LogMessage log;
                log.type = "#keaiAvoid";
                log.from = player;
                room->sendLog(log);

                return true;
        }

        return false;
    }
};

//ÈÈÐÄ
class rexin: public DrawCardsSkill{
public:
    rexin():DrawCardsSkill("rexin"){

    }

    virtual int getDrawNum(ServerPlayer *player, int n) const{
        Room *room = player->getRoom();
        QList<ServerPlayer *> players = room->getAlivePlayers(), targets;
         foreach(ServerPlayer *player, players){
             if(player->isWounded())
                 targets << player;
     }

         if(!targets.isEmpty()){
             int m = n;
             while(m > n-2 && !targets.isEmpty() && room->askForSkillInvoke(player, objectName())){
                 room->playSkillEffect(objectName());
                 ServerPlayer *target = room->askForPlayerChosen(player, targets, "rexin");
                 if(target != NULL){
                     RecoverStruct recover;
                     recover.who = player;
                     target->getRoom()->recover(target, recover);
                     targets.removeOne(target);
                     m--;
                 }else
                     break;
            }
             return m;
        }
            return n;
    }
};

//ÁÒÑª
class liexue: public TriggerSkill{
public:
    liexue():TriggerSkill("liexue"){
        events << Predamaged;
    }

    virtual bool triggerable(const ServerPlayer *target) const{
        return true;
    }

    virtual bool trigger(TriggerEvent , ServerPlayer *player, QVariant &data) const{

        Room *room = player->getRoom();
        ServerPlayer *zhifeng = room->findPlayerBySkillName(objectName());

        if(zhifeng == NULL || !zhifeng->faceUp() || zhifeng->hasFlag("chengdanle") || zhifeng->getPhase() == Player::Play)
                return false;

            DamageStruct damage = data.value<DamageStruct>();

            if(zhifeng == damage.to) return false;

            player->tag["LiexueDamage"] = QVariant::fromValue(damage);
            if(zhifeng->askForSkillInvoke(objectName(), data)){
                zhifeng->setFlags("chengdanle");
                DamageStruct damage = player->tag["LiexueDamage"].value<DamageStruct>();
                damage.to = zhifeng;
                damage.chain = true;
                room->damage(damage);

                if(zhifeng->isAlive()){
                    ServerPlayer *playerr = room->getCurrent();
                    room->setCurrent(zhifeng);
                    zhifeng->clearHistory();
                    room->getThread()->trigger(TurnStart, zhifeng);
                    zhifeng->turnOver();
                    zhifeng->setFlags("-chengdanle");
                    room->setCurrent(playerr);
                return true;
            }

            }

            return false;
    }
};

//Ò»Ò¹
YiyeCard::YiyeCard(){
    once = true;
}

bool YiyeCard::targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const{
    return targets.isEmpty();
}

void YiyeCard::onEffect(const CardEffectStruct &effect) const{
    Room *room = effect.from->getRoom();
    int n = effect.from->getLostHp();
    room->playSkillEffect("yiye");
    effect.to->turnOver();
    effect.to->drawCards(n);
    effect.from->drawCards(n);
    effect.from->turnOver();
}

class yiyeViewAsSkill: public ZeroCardViewAsSkill{
public:
    yiyeViewAsSkill():ZeroCardViewAsSkill("yiye"){
    }

    virtual const Card *viewAs() const{
        return new YiyeCard;
    }

protected:
    virtual bool isEnabledAtPlay(const Player *player) const{
        return false;
    }

    virtual bool isEnabledAtResponse(const Player *player, const QString &pattern) const{
        return  pattern == "@@yiye";
    }
};

class yiye:public PhaseChangeSkill{
public:
    yiye():PhaseChangeSkill("yiye"){
        view_as_skill = new yiyeViewAsSkill;
    }

    virtual bool onPhaseChange(ServerPlayer *sezhang) const{
        Room *room = sezhang->getRoom();
        if(sezhang->getPhase() == Player::Finish){
            if(room->askForUseCard(sezhang, "@@yiye", "@Yiye-card"))
                room->playSkillEffect("yiye");
        }
        return false;
    }
};

//Èë¹É
RuguCard::RuguCard(){
    target_fixed = true;
}

void RuguCard::use(Room *room, ServerPlayer *sezhang, const QList<ServerPlayer *> &targets) const{
    QString result = room->askForChoice(sezhang, "rugu", "b0+b1");
    int blood = 0;
    if(result == "b0") blood = 0;
    else if(result == "b1") blood = 1;
    //else if(result == "b2") blood = 2;
    //else if(result == "b3") blood = 3;
    room->broadcastInvoke("animate", "lightbox:$rugu");
    room->playSkillEffect("rugu");

    int cards = this->getSubcards().length();
    sezhang->loseMark("@share");
    room->throwCard(this);
    room->loseHp(sezhang, blood);
    if(sezhang->getHp() > 0){
        room->getThread()->delay();

        JudgeStruct display;
        display.pattern = QRegExp("(.*):(spade|club):(.*)");
        display.good = false;
        display.reason = "rugu";
        display.who = sezhang;

        room->display(display);

        if(!display.isBad()){
            LogMessage log;
            log.type = "#rugusuccess";
            log.from = sezhang;
            room->sendLog(log);

            room->gainMaxHp(sezhang, blood);

            RecoverStruct recover;
            recover.who = sezhang;
            recover.recover = blood * 2;
            room->recover(sezhang, recover);

            sezhang->drawCards(cards * 2);
        }else if(display.isBad()){
            LogMessage log;
            log.type = "#rugufail";
            log.from = sezhang;
            room->sendLog(log);
        }
    }
}


class Rugu: public ViewAsSkill{
public:
    Rugu(): ViewAsSkill("rugu"){
        frequency = Limited;
    }

    virtual bool isEnabledAtPlay(const Player *player) const{
        return player->getMark("@share") >= 1;
    }

    virtual bool viewFilter(const QList<CardItem *> &selected, const CardItem *to_select) const{
        return true;
    }

    virtual const Card *viewAs(const QList<CardItem *> &cards) const{
        if(cards.isEmpty())
            return NULL;

        RuguCard *card = new RuguCard;
        card->addSubcards(cards);
        return card;
    }
};

//ÑýÃÄ
class yaomei:public TriggerSkill{
public:
    yaomei():TriggerSkill("yaomei"){
        events << PhaseChange << FinishJudge;
    }

    virtual bool trigger(TriggerEvent event, ServerPlayer *caiwei, QVariant &data) const{
       if(event == PhaseChange && caiwei->getPhase() == Player::Draw && caiwei->askForSkillInvoke(objectName())){
            Room *room = caiwei->getRoom();

            QString suit = room->askForSuits(caiwei);
            while(true/*caiwei->askForSkillInvoke(objectName())*/){
                caiwei->setFlags("yaomei");

                room->playSkillEffect(objectName());

                JudgeStruct judge;
                judge.pattern = QRegExp(suit);
                judge.good = true;
                judge.reason = objectName();
                judge.who = caiwei;

                room->judge(judge);
                caiwei->obtainCard(judge.card);

                if(judge.isBad()){
                    room->setEmotion(caiwei, "bad");
                    break;
                }
            }
            caiwei->setFlags("-yaomei");
            return true;
        }else if(event == FinishJudge){
            if(caiwei->hasFlag("yaomei")){
                JudgeStar judge = data.value<JudgeStar>();
                if (judge->isGood()){
                    caiwei->obtainCard(judge->card);
                    return true;
                }
            }
        }

     return false;
    }
};

//ÃÆÉ§
class mensao: public PhaseChangeSkill{
public:
    mensao():PhaseChangeSkill("mensao"){
    }

    virtual bool onPhaseChange(ServerPlayer *caiwei) const{
        Room *room = caiwei->getRoom();

        if(caiwei->getPhase() == Player::Finish){
            if(caiwei->isWounded() && caiwei->askForSkillInvoke("mensao")){
                room->playSkillEffect(objectName());
                RecoverStruct recover;
                recover.who = caiwei;
                room->recover(caiwei, recover);
                caiwei->turnOver();
            }
        }
        return false;
    }
};

class jiejian: public TriggerSkill{
public:
    jiejian():TriggerSkill("jiejian"){
        events << Predamaged;

        frequency = Compulsory;
    }

    virtual bool trigger(TriggerEvent event, ServerPlayer *player, QVariant &data) const{
        Room *room = player->getRoom();

        if(event == Predamaged){
            if((!player->hasSkill("jiejian")) || (player->faceUp()))
                return false;

            room->playSkillEffect(objectName());

            DamageStruct damage = data.value<DamageStruct>();
                LogMessage log;
                log.type = "#jiejianAvoid";
                log.from = player;
                room->sendLog(log);

                return true;
        }

        return false;
    }
};

class gaoao: public TriggerSkill{
public:
    gaoao():TriggerSkill("gaoao"){
        events << Predamaged;

        frequency = Compulsory;
    }

    virtual bool trigger(TriggerEvent event, ServerPlayer *player, QVariant &data) const{
        Room *room = player->getRoom();

        if(event == Predamaged){
            DamageStruct damage = data.value<DamageStruct>();
            if((!player->hasSkill("gaoao")) || (player->faceUp()) || (!damage.nature == 'N'))
                return false;

            room->playSkillEffect(objectName());
                LogMessage log;
                log.type = "#gaoaoAvoid";
                log.from = player;
                room->sendLog(log);

                return true;
        }

        return false;
    }
};

//ÊõÕ¬
class shuzhai: public DrawCardsSkill{
public:
    shuzhai():DrawCardsSkill("shuzhai"){

    }
    virtual int getDrawNum(ServerPlayer *yuanlin, int n) const{
        /*int m = 0;
        QList<const Card *> equipcards = yuanlin->getEquips();
        foreach(const Card *card, equipcards){
            card->getNumber();
            m++;
        }*/
        Room *room = yuanlin->getRoom();
        if(yuanlin->getWeapon()){
        int m = yuanlin->getWeapon()->getRange();
        int b = yuanlin->getHp();
        int num = qAbs(m-b);
        LogMessage log;
        log.type = "#shuzhaiDraw";
        log.from = yuanlin;
        log.arg = QString::number(num);
        room->sendLog(log);

        room->playSkillEffect("shuzhai", num);

            return n + num;
        }else
            return n;
        }
};

//±ùÑÕ
class bingyan:public ViewAsSkill{
public:
    bingyan():ViewAsSkill("bingyan"){
    }

    virtual bool viewFilter(const QList<CardItem *> &selected, const CardItem *to_select) const{

        return !to_select->isEquipped() && to_select->getFilteredCard()->isBlack();

    }

    virtual const Card *viewAs(const QList<CardItem *> &cards) const{
        if(cards.length() == 2){
            const Card *first = cards.first()->getCard();
            Collateral *cc = new Collateral(first->getSuit(), 0);
            cc->addSubcards(cards);
            cc->setSkillName(objectName());
            return cc;
        }else
            return NULL;
    }
};

//ÒþÈÌ
class yinren:public TriggerSkill{
public:
    yinren():TriggerSkill("yinren"){
        frequency = Frequent;
        events << CardUsed << CardResponsed;
    }

    virtual bool trigger(TriggerEvent event, ServerPlayer *hongmeng, QVariant &data) const{
        CardStar card = NULL;
        if(event == CardUsed){
            CardUseStruct use = data.value<CardUseStruct>();
            card = use.card;
        }else if(event == CardResponsed)
            card = data.value<CardStar>();

        if(card->inherits("Slash") || card->inherits("Jink")){
           Room *room = hongmeng->getRoom();
            if(room->askForSkillInvoke(hongmeng, objectName())){
                room->playSkillEffect(objectName());
                hongmeng->drawCards(1);
            }
        }

        return false;
    }
};

//Ñ¸½Ý
class xunjie: public TriggerSkill{
public:
    xunjie():TriggerSkill("xunjie"){
        events << Damage;
    }

    virtual bool trigger(TriggerEvent , ServerPlayer *player, QVariant &data) const{
        DamageStruct damage = data.value<DamageStruct>();
        Room *room = player->getRoom();
        room->setPlayerFlag(player, "-drank");
        damage.to->setFlags("shaguole");

        if(damage.card && damage.card->inherits("Slash")){
            Room *room = player->getRoom();
            QList<ServerPlayer *> players = room->getOtherPlayers(damage.to), targets;
             foreach(ServerPlayer *player, players){
                 if(damage.to->canSlash(player) && !player->hasFlag("shaguole"))
                     targets << player;
                     targets.removeOne(damage.from);
             }

             if(!targets.isEmpty()){
                 if(room->askForSkillInvoke(player, objectName(), data)){
                room->playSkillEffect(objectName());


                        ServerPlayer *target = room->askForPlayerChosen(damage.from, targets, "xunjie");
                        target->setFlags("shaguole");
                        room->cardEffect(new Slash(Card::NoSuit, 0), player, target);

                }
            }
        }
        QList<ServerPlayer *> players = room->getAlivePlayers();
                foreach(ServerPlayer *player, players){
            player->setFlags("-shaguole");
         }

        return false;
    }
};

//Ð¡Ð¡
class xiaoxiao: public TriggerSkill{
public:
    xiaoxiao():TriggerSkill("xiaoxiao"){
        events << Dying;
    }

    virtual bool trigger(TriggerEvent , ServerPlayer *player, QVariant &) const{
        Room *room = player->getRoom();
            if(room->askForSkillInvoke(player, objectName())){
                JudgeStruct judge;
                judge.pattern = QRegExp("(.*):(club|heart|diamond):(.*)");
                judge.good = true;
                judge.reason = "xiaoxiao";
                judge.who = player;

                int num = player->getHp();

                room->judge(judge);

                if(judge.isBad()){
                    LogMessage log;
                    log.type = "#xiaoxiaoRecoverbad";
                    log.from = player;
                    room->sendLog(log);
                }
                else{
                room->playSkillEffect(objectName());
                RecoverStruct recover;
                recover.who = player;
                recover.recover = 1-num;
                room->recover(player, recover);
            }
        }
        return false;
    }
};

//½É¾í
JiaojuanCard::JiaojuanCard()
{
    once = true;
    setObjectName("JiaojuanCard");
}

bool JiaojuanCard::targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const{
    if(!targets.isEmpty())
        return false;

    return Self->inMyAttackRange(to_select) && !to_select->isKongcheng();
}

void JiaojuanCard::onEffect(const CardEffectStruct &effect) const{
    QString suit_str = "no_suit";

    foreach(int card_id, effect.card->getSubcards()){
        const Card *card = Sanguosha->getCard(card_id);
        suit_str = card->getSuitString();
    }

    QString pattern = QString(".%1").arg(suit_str.at(0).toUpper());
    QString prompt = QString("@jiaojuan:%1::%2").arg(effect.from->getGeneralName()).arg(suit_str);

    Room *room = effect.from->getRoom();

    room->playSkillEffect(objectName());

    const Card *card = room->askForCard(effect.to, pattern, prompt);
    if(card){
        room->showCard(effect.to, card->getEffectiveId());
        effect.from->obtainCard(card);
    }else{

    DamageStruct damage;
    damage.card = NULL;
    damage.from = effect.from;
    damage.to = effect.to;

    room->damage(damage);
}
}

class jiaojuan: public ViewAsSkill{
public:
    jiaojuan():ViewAsSkill("jiaojuan"){

    }

    virtual bool isEnabledAtPlay(const Player *player) const{
        return ! player->hasUsed("JiaojuanCard");
    }

    virtual bool viewFilter(const QList<CardItem *> &selected, const CardItem *to_select) const{
        return selected.isEmpty() && !to_select->isEquipped();
    }

    virtual const Card *viewAs(const QList<CardItem *> &cards) const{
        if(cards.isEmpty())
            return NULL;
        else if(cards.length() == 1){
            JiaojuanCard *card = new JiaojuanCard();
            card->addSubcards(cards);
            return card;
        }else
            return NULL;
    }
};

CaichuangPackage::CaichuangPackage()
    :Package("Caichuang")
{
    //²Ä´´É± µ°ÊÆÁ¦
    General *shaolong, *tianjie, *zhiniang, *yujian, *ziming, *hanshi, *liying;

    shaolong = new General(this, "shaolong$", "dan");
    shaolong->addSkill(new danteng);
    shaolong->addSkill(new budan);

    tianjie = new General(this, "tianjie", "dan", 3);
    tianjie->addSkill(new Tiandao);
    tianjie->addSkill(new hengtian);

    zhiniang = new General(this, "zhiniang", "dan", 3);
    zhiniang->addSkill(new heiying);
    zhiniang->addSkill(new kaihei);

    yujian = new General(this, "yujian", "dan", 3);
    yujian->addSkill(new chenxing);
    yujian->addSkill(new Skill("yinpan", Skill::Compulsory));

    ziming = new General(this, "ziming", "dan");
    ziming->addSkill(new chixian);
    ziming->addSkill(new Skill("tifei", Skill::Compulsory));

    hanshi = new General(this, "hanshi", "dan", 8);
    hanshi->addSkill(new yijing);
    hanshi->addSkill("yanshe");
    hanshi->addSkill(new shiti);

    liying = new General(this, "liying", "dan", 5);
    liying->addSkill(new chenmo);
    liying->addSkill(new ChenmoStart);
    liying->addSkill(new ChenmoClear);
    liying->addSkill(new chenmodeadClear);

    related_skills.insertMulti("chenmo", "#chenmostart");
    related_skills.insertMulti("chenmo", "#chenmo-clear");
    related_skills.insertMulti("chenmo", "#chenmodead-clear");

    //²Ä´´É± ÉñÊÆÁ¦
    General *zhoushen, *fashen, *xiaowen, *dalao, *wshen, *xiongshen, *sunchen;

    zhoushen = new General(this, "zhoushen$", "shen", 3);
    zhoushen->addSkill(new shensheng);
    zhoushen->addSkill(new shenhui);
    zhoushen->addSkill(new jishen);

    fashen = new General(this, "fashen", "shen", 3);
    fashen->addSkill(new fengliu);
    fashen->addSkill(new boxue);

    xiaowen = new General(this, "xiaowen", "shen");
    xiaowen->addSkill(new baqi);

    dalao = new General(this, "dalao", "shen", 3);
    dalao->addSkill(new shence);
    dalao->addSkill(new guozhi);

    wshen = new General(this, "wshen", "shen");
    wshen->addSkill(new shenju);
    wshen->addSkill(new yingya);
    wshen->addSkill(new shenjuclear);
    wshen->addSkill(new juhun);

    related_skills.insertMulti("shenju", "#shenjuclear");

    xiongshen = new General(this, "xiongshen", "shen");
    xiongshen->addSkill(new xiongba);
    xiongshen->addSkill("mashu");

    sunchen = new General(this, "sunchen", "shen", 3, false);
    sunchen->addSkill(new xingchen);
    sunchen->addSkill(new xingchenDraw);
    sunchen->addSkill(new wushu);

    related_skills.insertMulti("xingchen", "#xingchen-draw");

    //²Ä´´É± »ùÊÆÁ¦
    General *saobi, *saohuo, *yudong, *eshen, *guanzhang, *xiaozhi, *zhonghao;

    saobi = new General(this, "saobi$", "ji", 3);
    saobi->addSkill(new xianju);
    saobi->addSkill(new zhengtai);
    saobi->addSkill(new jiyou);
    saobi->addSkill(new dyingjiyou);
    saobi->addSkill(new jiyoustart);

    related_skills.insertMulti("jiyou", "#dyingjiyou");
    related_skills.insertMulti("jiyou", "#jiyoustart");

    saohuo = new General(this, "saohuo$", "ji", 3);
    saohuo->addSkill(new baoju);
    saohuo->addSkill(new fuhei);
    saohuo->addSkill(new fuheiRetrial);
    saohuo->addSkill(new fasao);

    related_skills.insertMulti("fuhei", "#fuhei-retrial");

    yudong = new General(this, "yudong", "ji");
    yudong->addSkill(new yingba);
    yudong->addSkill(new yingbaClear);

    related_skills.insertMulti("yingba", "#yingba-clear");

    eshen = new General(this, "qifan", "ji");
    eshen->addSkill(new jike);
    eshen->addSkill(new qinfen);

    guanzhang = new General(this, "guanzhang", "ji", 3);
    guanzhang->addSkill(new xiee);
    guanzhang->addSkill(new yicaii);

    xiaozhi = new General(this, "xiaozhi", "ji");
    xiaozhi->addSkill(new xiaozhii);

    zhonghao = new General(this, "zhonghao", "ji", 3);
    zhonghao->addSkill(new haixiu);
    zhonghao->addSkill(new fanjia);
    zhonghao->addSkill(new zhongzhi);

    //²Ä´´É± ÈÌÊÆÁ¦
    General *xiaojie, *zhifeng, *sezhang, *caiwei, *yuanlin, *hongmeng, *xiaohao, *xiaozhou;

    xiaojie = new General(this, "xiaojie", "ren", 3);
    xiaojie->addSkill(new shoucai);
    xiaojie->addSkill(new keaiClear);
    xiaojie->addSkill(new keai);

    zhifeng = new General(this, "zhifeng", "ren");
    zhifeng->addSkill(new rexin);
    zhifeng->addSkill(new liexue);

    sezhang = new General(this, "sezhang", "ren", 3);
    sezhang->addSkill(new yiye);
    sezhang->addSkill(new Rugu);
    sezhang->addSkill(new MarkAssignSkill("@share", 1));
    sezhang->addSkill(new jiejian);

    caiwei = new General(this, "caiwei", "ren", 3, false);
    caiwei->addSkill(new yaomei);
    caiwei->addSkill(new mensao);
    caiwei->addSkill(new gaoao);

    yuanlin = new General(this, "yuanlin", "ren", 4, false);
    yuanlin->addSkill(new shuzhai);
    yuanlin->addSkill(new bingyan);

    hongmeng = new General(this, "hongmeng", "ren", 3);
    hongmeng->addSkill(new yinren);
    hongmeng->addSkill(new Skill("bisuo", Skill::Compulsory));

    xiaohao = new General(this, "xiaohao", "ren", 2);
    xiaohao->addSkill(new Skill("zhuanzhu", Skill::Compulsory));
    xiaohao->addSkill(new xunjie);
    xiaohao->addSkill(new xiaoxiao);

    xiaozhou = new General(this, "xiaozhou", "ren", 3);
    xiaozhou->addSkill(new jiaojuan);

    //²Ä´´É± $#@^&@@%$#%#@
    /*General *sibada, *zhazha;

    sibada = new General(this, "sibada", "shi");

    zhazha = new General(this, "zhazha", "dan", 1);
    zhazha->addSkill(new Skill("zhazha1", Skill::Compulsory));
    zhazha->addSkill("luoshen");
    zhazha->addSkill("xiaoji");
    zhazha->addSkill("kurou");
    zhazha->addSkill("jizhi");
    zhazha->addSkill("paoxiao");*/


    addMetaObject<TiandaoCard>();
    addMetaObject<JiaojuanCard>();
    addMetaObject<JishenCard>();
    addMetaObject<ChenxingCard>();
    addMetaObject<HaixiuJink>();
    addMetaObject<FanjiaCard>();
    addMetaObject<YiyeCard>();
    addMetaObject<RuguCard>();
    addMetaObject<ShoucaiCard>();
    addMetaObject<YingbaCard>();
    addMetaObject<ChenmoCard>();
    addMetaObject<XianjuCard>();
    addMetaObject<ZhengtaiCard>();
    addMetaObject<JiyouCard>();
    addMetaObject<DantengCard>();
    addMetaObject<ShenjuCard>();
    addMetaObject<YansheCard>();
    addMetaObject<Shite>();
    addMetaObject<ShenceCard>();
    addMetaObject<GuozhiCard>();
    addMetaObject<KaiheiCard>();

    skills << new jishenViewAsSkill;
    skills << new jiyouji;
    skills << new yansheViewAsSkill;
 }

ADD_PACKAGE(Caichuang)
