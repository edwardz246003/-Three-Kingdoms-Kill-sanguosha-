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
#include "generaloverview.h"
#include <QMessageBox>

//衡天
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

//天道
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
        return !to_select->isEquipped();
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
        JudgeStar judge = data.value<JudgeStar>();

        QStringList prompt_list;
        prompt_list << "@tiandao-card" << judge->who->objectName()
                << "" << judge->reason << judge->card->getEffectIdString();
        QString prompt = prompt_list.join(":");

        player->tag["Judge"] = data;
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

            room->loseHp(player);

        }

        return false;
    }
};

//黑影
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

//痴线
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

//尘星
ChenxingCard::ChenxingCard(){
    once = true;
}

void ChenxingCard::onEffect(const CardEffectStruct &effect) const{
    int n = subcardsLength();
    Room *room = effect.from->getRoom();
    QString choice = room->askForChoice(effect.from, "chenxing", "cxq+cxm");
    if(choice == "cxq"){
        room->askForDiscard(effect.to, "chenxing", n, false, true);
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

        if(yujian->getPhase() == Player::Finish && !yujian->isKongcheng()){
            if(room->askForUseCard(yujian, "@@chenxing", "@chenxing")){
                return true;
            }
        }
        return false;
    }
};

//隐磐
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

//神圣
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

//神诲
class shenhui:public TriggerSkill{
public:
    shenhui():TriggerSkill("shenhui"){
        frequency = Frequent;
        events << CardUsed << CardResponsed << FinishJudge;
    }
    virtual bool trigger(TriggerEvent , ServerPlayer *player, QVariant &data) const{
            return false;
    }
};

//祭神
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
            if(player->getKingdom() == "shen")
            room->attachSkillToPlayer(player, "jishenv");
        }
    }
};

//风流
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

//博学
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

//霸气
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

//熊霸
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

//星辰
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
            int n = sunchen->aliveCount();

            QString pattern = QString("(.*):(spade|club):([%1-9]|..|J|Q|K)").arg(n);
            if(n==9)
            {
             pattern = QString("(.*):(spade|club):(..|J|Q|K)").arg(n);
            }

            if (n==10)
            {
              pattern = QString("(.*):(spade|club):(J|Q|K)").arg(n);
            }
            JudgeStruct judge;
            judge.pattern = QRegExp(pattern);
            judge.good = true;
            judge.reason = objectName();
            judge.who = sunchen;

            room->judge(judge);
            if(!judge.isBad()){
            sunchen->gainMark("@xing", 1);
            sunchen->obtainCard(judge.card);

            QList<int> xings = sunchen->handCards().mid(sunchen->getHandcardNum() - 1);

            foreach(int card_id, xings)
                sunchen->addToPile("xings", card_id, false);
        }

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
            sunchen->loseMark("@xing",n);

            LogMessage log;
            log.type = "#XingchenDraw";
            log.from = sunchen;
            log.arg = QString::number(n);
            room->sendLog(log);
        }

        return false;
    }
};

//巫术
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

/*腹黑
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
};*/

//硬霸
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
                if(player->getMark("imba")){

                    player->removeMark("imba");

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

/*勤奋
class qinfen: public TriggerSkill{
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
};

//饥渴
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

};*/

//啸炙
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

//害羞
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

//返家
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
        if(zhonghao->getPhase() == Player::Finish){
            if(room->askForUseCard(zhonghao, "@@fanjia", "@Fanjia-card")){
                return false;
            }
        }

        return false;
    }
};

//中指
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

//收财
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

//可爱
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

//热心
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
                 RecoverStruct recover;
                 recover.who = player;
                 target->getRoom()->recover(target, recover);
                 targets.removeOne(target);
                 m--;
            }
             return m;
        }
            return n;
    }
};

//烈血
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
                    zhifeng->clearFlags();
                    room->setCurrent(playerr);
                return true;
            }

            }

            return false;
    }
};

//一夜
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
            if(room->askForUseCard(sezhang, "@@yiye", "@Yiye-card")){
                return false;
            }
        }

        return false;
    }
};

//入股
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

//妖媚
class yaomei:public TriggerSkill{
public:
    yaomei():TriggerSkill("yaomei"){
        events << PhaseChange << FinishJudge;
    }

    virtual bool trigger(TriggerEvent event, ServerPlayer *caiwei, QVariant &data) const{
       if(event == PhaseChange)
        {
        if(caiwei->getPhase() == Player::Draw){
            Room *room = caiwei->getRoom();
            if(!caiwei->askForSkillInvoke(objectName())){
                caiwei->clearFlags();
                return false;
            }

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

        }
    }else if(event == FinishJudge){
            if(caiwei->hasFlag("yaomei")){
                JudgeStar judge = data.value<JudgeStar>();
                if (judge->reason == "yaomei" && judge->isGood())
                    return true;

            }
        }

     return false;
    }
};

//闷骚
class mensao: public PhaseChangeSkill{
public:
    mensao():PhaseChangeSkill("mensao"){
    }

    virtual bool onPhaseChange(ServerPlayer *caiwei) const{
        Room *room = caiwei->getRoom();

        if(caiwei->getPhase() == Player::Finish && !caiwei->isKongcheng()){
            if(caiwei->isWounded() && caiwei->askForSkillInvoke("mensao")){
                room->playSkillEffect("mensao");
                RecoverStruct recover;
                recover.who = caiwei;
                room->recover(caiwei, recover);
                caiwei->turnOver();
                return true;
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

//冰颜
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

//隐忍
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

//迅捷
class xunjie: public TriggerSkill{
public:
    xunjie():TriggerSkill("xunjie"){
        events << Damage;
    }

    virtual bool trigger(TriggerEvent , ServerPlayer *player, QVariant &data) const{
        DamageStruct damage = data.value<DamageStruct>();
        player->clearFlags();
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
        Room *room = player->getRoom();
        QList<ServerPlayer *> players = room->getAlivePlayers();
                foreach(ServerPlayer *player, players){
            player->clearFlags();
         }

        return false;
    }
};

//小小
class xiaoxiao: public TriggerSkill{
public:
    xiaoxiao():TriggerSkill("xiaoxiao"){
        events << Dying << AskForPeachesDone;
    }

    virtual bool trigger(TriggerEvent event, ServerPlayer *player, QVariant &) const{
        Room *room = player->getRoom();

        if(event == Dying){
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
               // log.arg = QString::number(1);
                    room->sendLog(log);

                //room->playSkillEffect("xiaoxiao", 2);
                }
                else{
                room->playSkillEffect(objectName());
                RecoverStruct recover;
                recover.who = player;
                recover.recover = 1-num;
                room->recover(player, recover);
            }

                //player->setHp(1);

                //room->playSkillEffect("xiaoxiao", 3);

            return false;
        }
        }
        return false;
    }
};

//缴卷
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
    //材创杀 蛋势力
    General *shaolong, *tianjie, *zhiniang, *yujian, *ziming, *hanshi, *liying;

    shaolong = new General(this, "shaolong$", "dan");
    // shaolong->addSkill(new danteng);
    // shaolong->addSkill(new didiao);
    // shaolong->addSkill(new budan);

    tianjie = new General(this, "tianjie", "dan", 3);
    tianjie->addSkill(new Tiandao);
    tianjie->addSkill(new hengtian);

    zhiniang = new General(this, "zhiniang", "dan", 3);
    zhiniang->addSkill(new heiying);
    // zhiniang->addSkill(new kaihei);

    yujian = new General(this, "yujian", "dan", 3);
    yujian->addSkill(new chenxing);
    yujian->addSkill(new Skill("yinpan", Skill::Compulsory));

    ziming = new General(this, "ziming", "dan");
    ziming->addSkill(new chixian);
    ziming->addSkill(new Skill("tifei", Skill::Compulsory));

    hanshi = new General(this, "hanshi", "dan", 8);
    //  hanshi->addSkill(new zise);
    //  hanshi->addSkill(new yijing);
    //   hanshi->addSkill(new yanshe);
    //  hanshi->addSkill(new shiti);

    liying = new General(this, "liying", "dan", 3);
    //liying->addSkill(new chenmo);

    //材创杀 神势力
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
    // dalao->addSkill(new shenji);
    // dalao->addSkill(new guozhi);

    wshen = new General(this, "wshen", "shen");
    //  wshen->addSkill(new shenju);
    //  wshen->addSkill(new yingya);
    //  wshen->addSkill(new juhun);

    xiongshen = new General(this, "xiongshen", "shen");
    xiongshen->addSkill(new xiongba);
    //  xiongshen->addSkill(new hongzhi);

    sunchen = new General(this, "sunchen", "shen", 3, false);
    sunchen->addSkill(new xingchen);
    sunchen->addSkill(new xingchenDraw);
    sunchen->addSkill(new wushu);

    related_skills.insertMulti("xingchen", "#xingchen-draw");

    //材创杀 基势力
    General *saobi, *saohuo, *yudong, *eshen, *guanzhang, *xiaozhi, *zhonghao;

    saobi = new General(this, "saobi$", "ji", 3);

    saohuo = new General(this, "saohuo$", "ji", 3);
    //saohuo->addSkill(new fuhei);
    //saohuo->addSkill(new fuheiRetrial);

    related_skills.insertMulti("fuhei", "#fuhei-retrial");

    yudong = new General(this, "yudong", "ji");
    yudong->addSkill(new yingba);
    yudong->addSkill(new yingbaClear);

    related_skills.insertMulti("yingba", "#yingba-clear");

    eshen = new General(this, "qifan", "ji");
    //eshen->addSkill(new jike);
    //eshen->addSkill(new qinfen);

    guanzhang = new General(this, "guanzhang", "ji", 3);

    xiaozhi = new General(this, "xiaozhi", "ji");
    xiaozhi->addSkill(new xiaozhii);

    zhonghao = new General(this, "zhonghao", "ji", 3);
    zhonghao->addSkill(new haixiu);
    zhonghao->addSkill(new fanjia);
    zhonghao->addSkill(new zhongzhi);

    //材创杀 忍势力
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

    yuanlin = new General(this, "yuanlin", "ren", 3, false);
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

    /*材创杀 $#@^&@@%$#%#@
    General *sibada, *zhazha;

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

    skills << new jishenViewAsSkill;
 }

ADD_PACKAGE(Caichuang)
