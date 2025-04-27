#ifndef LOLISTORM_LOFLOATERSPOOF_H
#define LOLISTORM_LOFLOATERSPOOF_H

#include "llfloater.h"

class LOFloaterSpoof : public LLFloater
{
public:
    LOFloaterSpoof(const LLSD& key);
    /*virtual*/ ~LOFloaterSpoof();

    /*virtual*/ bool postBuild();

    void update_labels();
};

#endif // LOLISTORM_LOFLOATERSPOOF_H
