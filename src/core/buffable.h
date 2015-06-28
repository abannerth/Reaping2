#ifndef INCLUDED_CORE_BUFFABLE_H
#define INCLUDED_CORE_BUFFABLE_H

template<typename T>
class Limited
{
public:
    T GetMax() const;
    Limited<T>& SetMax(T max);
    T GetMin() const;
    Limited<T>& SetMin(T min);
    T Get() const;
    Limited<T>& Set(T val);
    Limited();
    Limited<T>& Init(T val, T min, T max);
private:
    T mMax;
    T mMin;
    T mVal;
};

template<typename T>
Limited<T>& Limited<T>::Init(T val, T min, T max)
{
    SetMin(min).SetMax(max).Set(val);
    return *this;
}

template<typename T>
Limited<T>::Limited()
{
}

template<typename T>
Limited<T>& Limited<T>::Set(T val)
{
    mVal=std::max(mMin,std::min(mMax,val));
    return *this;
}

template<typename T>
T Limited<T>::Get() const
{
    return mVal;
}

template<typename T>
Limited<T>& Limited<T>::SetMin(T min)
{
    mMin=min;
    return *this;
}

template<typename T>
T Limited<T>::GetMin() const
{
    return mMin;
}

template<typename T>
Limited<T>& Limited<T>::SetMax(T max)
{
    mMax=max;
    return *this;
}

template<typename T>
T Limited<T>::GetMax() const
{
    return mMax;
}

template<typename T>
class Buffable
{
public:
    T Get() const;
    Limited<T> mBase;
    Limited<double> mPercent;
    Limited<T> mFlat;
};

template<typename T>
T Buffable<T>::Get() const
{
    return mBase.Get()
        +(T)(mBase.Get()*(mPercent.Get()/100.0))
        +mFlat.Get();
}

#endif//INCLUDED_CORE_BUFFABLE_H