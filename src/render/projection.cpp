#include "i_render.h"

const glm::mat4& Projection::GetMatrix() const
{
	return mMatrix;
}

glm::vec3 Projection::Unproject(glm::vec3& Vec) const
{
	glm::vec3 V(Vec.x,mViewport.Height-Vec.y,Vec.z);
	return glm::unProject(V,mIdentity,mMatrix,mViewportTransf);
}

const Viewport& Projection::GetViewport() const
{
	return mViewport;
}

void Projection::Setup( float bottom,float top,ViewMode vm/*=VM_DynamicRatio*/,float nearVal/*=1.0f*/,float farVal/*=-1.0f*/ )
{
	mBottom=bottom;
	mTop=top;
	mViewMode=vm;
	if(mViewMode==VM_DynamicRatio)
	{
		mLeft=mRatio*mBottom;
		mRight=mRatio*mTop;
	}
	else
	{
		mLeft=mBottom;
		mRight=mTop;
	}
	mNearVal=nearVal;
	mFarVal=farVal;
	SetupMatrices();
}

void Projection::OnWindowResizeEvent(const WindowResizeEvent& Event)
{
	Resize(Event.Width,Event.Height);
	Setup(mBottom,mTop,mViewMode,mNearVal,mFarVal);
}

void Projection::Resize(int Width, int Height)
{
	mViewport.X=0;
	mViewport.Y=0;
	mViewport.Width=Width;
	mViewport.Height=Height;
	mRatio=mViewport.Height?(float)mViewport.Width/mViewport.Height:1.f;
}

void Projection::SetupMatrices()
{
	mViewportTransf=glm::vec4(mViewport.X,mViewport.Y,mViewport.Width,mViewport.Height);
	mMatrix=glm::ortho(mLeft,mRight,mBottom,mTop,mNearVal,mFarVal);
	mInverseMatrix=glm::transpose(mMatrix);
}

Projection::Projection( float bottom,float top,ViewMode vm/*=VM_DynamicRatio*/,float nearVal/*=1.0f*/,float farVal/*=-1.0f*/ )
{
	mWindowResizeId=EventServer<WindowResizeEvent>::Get().Subscribe(boost::bind(&Projection::OnWindowResizeEvent,this,_1));
	int w,h;
	Window::Get().GetWindowSize(w,h);
	Resize(w,h);
	Setup(bottom,top,vm,nearVal,farVal);
}

const glm::mat4 Projection::mIdentity=glm::mat4(1.0);
