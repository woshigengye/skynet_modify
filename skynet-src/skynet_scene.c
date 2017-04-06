#include "skynet.h"

#include "skynet_scene.h"
#include <stdlib.h>
#include <string.h>


#define LENGX 2
#define LENGY 2

//同轴 opp 大的
#define LG(x) (x+2)  

struct AABB{
    int uid;
    int pos[4]; //左下右上
    struct AABB* next[4]; //每个维度的双向链表 0,1 代表xy轴正方向
} ;
typedef struct AABB AABB;

struct AABBPAIRS{
    AABB* a;
    AABB* b;
    struct AABBPAIRS* next;
};
typedef struct AABBPAIRS PAIRS;

//#define NUSER  10
struct AOI_SPACE{
    AABB* list_head[2];
    PAIRS* pairs_head[2];
    int allUser;
    //AABB* G_user[NUSER];
};
typedef struct AOI_SPACE AOI_SPACE;


static AOI_SPACE*  Aoi= NULL;


AOI_SPACE* aoispace_create(){
    AOI_SPACE* aoispace = (AOI_SPACE* )skynet_malloc(sizeof(AOI_SPACE));

    memset(aoispace,0,sizeof(*aoispace));
    aoispace->allUser = 0;

    int i=0;
    for(i=0;i<2;i++){
        aoispace->list_head[i]=NULL;
        aoispace->pairs_head[i] = NULL;
    }
    
    return aoispace;
}

void aoispace_release(AOI_SPACE* aoispace)
{
    if (aoispace ==NULL) return;

    int i;
    for(i=0;i<2;i++){
        PAIRS* pairs = aoispace->pairs_head[i];
        while(pairs!=NULL){
            PAIRS* temp = pairs;
            pairs = pairs->next;
            skynet_free(temp);
        }
    
        AABB* list = aoispace->list_head[i];
        while(list!=NULL){
            AABB* temp = list;
            list = list->next[0];
            skynet_free(temp);
        }
    }
    skynet_free(aoispace);
}

// 按照uid加入pairs
void add_pairs(PAIRS** pairs, AABB* a, AABB* b, int posid)
{
    PAIRS* first = pairs[posid];

    PAIRS* pa = (PAIRS*) skynet_malloc(sizeof(PAIRS));
    memset(pa, 0, sizeof(*pa));
    pa->a = a->uid>b->uid? b:a;
    pa->b = a->uid>b->uid? a:b;
    pa->next = first;

    pairs[posid] = pa;
}

void sweep(PAIRS** pairs, AABB* list, int posid)
{
    AABB* first = list;
    if (first == NULL) return;
    
    AABB* next = list->next[posid];
   
    while(next !=NULL){
          //  skynet_error(NULL, "a=%d, b=%d", first->pos[LG(posid)], next->pos[posid]);
        if (first->pos[LG(posid)]> next->pos[posid]){
            //加入PAIRS列表
            //skynet_error(NULL, "uid1 = %d uid2 = %d", first->uid, next->uid);  
            add_pairs(pairs, first, next, posid); 
            next = next->next[posid];
        }else{
            first = first->next[posid];
            next = first->next[posid];
        }
    }
}

// assume that to collision is to see
int cmp(AABB* a, AABB* b)
{
    return 1;
}

//进行一次所有玩家 aoi信息刷新
void aoispace_refresh(AOI_SPACE* aoispace ){
    AABB** list = aoispace->list_head;
    PAIRS** pairs = aoispace->pairs_head;
    pairs[0] = NULL;
    pairs[1] = NULL;

    int i=0;
   
    // possibly collide
    for(i=0;i<2;i++){
        sweep(pairs, list[i], i);
    }

    // 取出两条pairs中相同的pairs，进一步比较
    PAIRS* p = pairs[0];
    PAIRS* q = pairs[1];
    while(p!=NULL){
        while(q!=NULL){
            if((p->a)->uid ==(q->a)->uid && (p->b)->uid ==(q->b)->uid){
                //进一步比较
                int res;
                if (res = cmp(p->a, p->b)){
                    //skynet_error(NULL, "a =%d meet b=%d, status code=%d", (p->a)->uid, (p->b)->uid, res);
                }
            }
            q = q->next;
        }
        
        p = p->next;
        q = pairs[1];
    }

    // release the memory of AOI_SPACE->PAIRS
    for(i=0;i<2;i++){
        PAIRS* tofree = aoispace->pairs_head[i];
        PAIRS* tmp;
        while(tofree!=NULL){
            tmp=tofree->next;
            skynet_free(tofree);
            tofree = tmp;
        }
    }

}
   
int is_user_in_scene(int uid, AABB** pUser)
{

    AABB* first = Aoi->list_head[0];
    while(first!=NULL){
        if (first->uid == uid){
            *pUser = first;
            return 1;    
        }
        first = first->next[0];
    }
    return 0;
}

void adduser(AABB** list, AABB* aabb, int posid){

    if (list==NULL){
        // adduser 第一个不能在此加，会引起coredump
        skynet_error(NULL, "exception!");
    }else{
        AABB* temp=*list;
        AABB* previous;
        if (temp->pos[posid]>=aabb->pos[posid]){
            aabb->next[posid]=*list;
            (*list)->next[LG(posid)] = aabb;
            *list = aabb;
            return;
        }


        while(temp->pos[posid]<aabb->pos[posid]){
            if(temp->next[posid]==NULL){
                previous=temp;
                break;
            }else{
                if(    (temp->next[posid])->pos[posid] >= aabb->pos[posid]){
                    previous=temp;
                    break;
                }else{
                    temp = temp->next[posid];
                }
            }
        }

        skynet_error(NULL, "temp = %d", aabb->uid);
        temp =previous->next[posid];
        previous->next[posid] = aabb;
        aabb->next[LG(posid)] = previous;
        aabb->next[posid] = temp;
        
        if(temp!=NULL) temp->next[LG(posid)] = aabb;
    }
}

//加入aoi
int aoispace_adduser(AOI_SPACE* aoispace, int uid, int x, int y){
  
    if (is_user_in_scene(uid, NULL)) {
        skynet_error(NULL, "uid=%d already in scene ", uid);
        return 0;
    }
   
    AABB* aabb = (AABB* )skynet_malloc(sizeof(AABB));
    memset(aabb ,0,sizeof(*aabb));
    
    aabb->uid = uid;
    aabb->pos[0] = x-LENGX/2;
    aabb->pos[1] = y-LENGY/2;
    aabb->pos[2] = x+LENGX/2;
    aabb->pos[3] = y+LENGY/2;
    
    int i;
    for( i=0;i<4;i++){
        aabb->next[i] = NULL;
    }

    for(i=0;i<2;i++){
        if (aoispace->list_head[i]==NULL){
            aoispace->list_head[i] = aabb;
        }else{
            adduser(&aoispace->list_head[i], aabb, i);
        }
    }
    
    aoispace->allUser = aoispace->allUser + 1;
    return 1;
}

void remove_user(AABB** list, AABB* aabb, int posid)
{
    AABB* prev = aabb->next[LG(posid)];
    AABB* next = aabb->next[posid];

    if (prev!=NULL){
        prev->next[posid]=next;
    }
    if (next!=NULL){
        next->next[LG(posid)]=prev;
    }

    aabb->next[LG(posid)] = NULL;
    aabb->next[posid] = NULL;

    if (prev == NULL){
        list[posid] = next;
    }
    
    if (Aoi->allUser == 1){
        list[posid] = NULL;
    }
}

int aoispace_removeuser(AOI_SPACE* aoispace, int uid)
{
    
    //find AABB
    AABB* pUser ;
    if (!is_user_in_scene(uid, &pUser)){
        skynet_error(NULL, "uid=%d no found", uid);
        return 0;
    }
   
    if (pUser ==NULL){
        skynet_error(NULL, "pUser is NULL");
        return 0;
    }
    
    int i;
    for(i=0;i<2;i++){
        remove_user(aoispace->list_head, pUser, i);
    }
    
    aoispace->allUser = aoispace->allUser - 1;
    skynet_free(pUser);
    return 1;
}

int skynet_debugscene()
{
    skynet_error(NULL, "alluser = %d", Aoi->allUser);
    int i=0;
    for(i=0;i<2;i++){
        //skynet_error(NULL, " %i list:", i);
        AABB* head = Aoi->list_head[i];
        while (head!=NULL){
            skynet_error(NULL, "%d ---> %d, %d", head->uid, head->pos[0] + LENGX/2, head->pos[1] + LENGY/2 );
            head = head->next[i];
        }
        //skynet_error(NULL, "end %d list", i);
    }
    
    skynet_error(NULL, "\n######", i);
    return 1;
}
       
int skynet_leavescene( int uid)
{
    if (!aoispace_removeuser(Aoi, uid)){
        skynet_error(NULL, "skynet_removescene error, uid = %d", uid);
    }
    aoispace_refresh(Aoi);
}
int skynet_enterscene(int uid, int x,int y)
{
    if (!aoispace_adduser(Aoi, uid, x, y)){
        skynet_error(NULL, "skynet_enterscene error, uid = %d", uid);
    }
    aoispace_refresh(Aoi);
}
int skynet_movescene(int uid, int x,int y)
{
    if (!aoispace_removeuser(Aoi, uid)){
        skynet_error(NULL, "skynet_removescene error, uid = %d", uid);
        return 1;
    }

    if (!aoispace_adduser(Aoi, uid, x, y)){
        skynet_error(NULL, "skynet_movescene error, uid = %d", uid);
    }
    aoispace_refresh(Aoi);

}

void skynet_aoi_init()
{
    aoispace_release(Aoi);
    Aoi = aoispace_create();
}

