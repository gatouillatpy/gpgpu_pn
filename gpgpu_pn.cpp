
#include <windows.h>
#include <mmsystem.h>
#include <d3dx9.h>
#include <strsafe.h>
#include <iostream>

using namespace std;



HINSTANCE hInstance;

LPDIRECT3D9 lpD3D = NULL;

LPDIRECT3DDEVICE9 lpD3DDevice = NULL;

LPDIRECT3DVERTEXDECLARATION9 lpVertexDeclaration = NULL;

LPDIRECT3DVERTEXBUFFER9 lpVertexBuffer = NULL;
LPDIRECT3DINDEXBUFFER9 lpIndexBuffer = NULL;

LPDIRECT3DSURFACE9 lpRenderTarget = NULL;
LPDIRECT3DSURFACE9 lpRenderSurface = NULL;

LPDIRECT3DVERTEXSHADER9 lpVertexShader = NULL;
LPDIRECT3DPIXELSHADER9 lpPixelShader = NULL;

LPD3DXBUFFER lpError ;

LPD3DXBUFFER lpVSCode ;
LPD3DXBUFFER lpPSCode ;

LPD3DXCONSTANTTABLE lpVSData ;
LPD3DXCONSTANTTABLE lpPSData ;

DWORD nMax;
DWORD nRoot;

DWORD nWidth;
DWORD nHeight;

DWORD nCount;

FLOAT* pFactor;

FLOAT vModulo[4];

CONST DWORD nConstLimit = 10;



HRESULT Initialize( HWND hWnd )
{
	if ( NULL == ( lpD3D = Direct3DCreate9( D3D_SDK_VERSION ) ) )
        return E_FAIL;

    D3DPRESENT_PARAMETERS d3dpp;
    ZeroMemory( &d3dpp, sizeof(d3dpp) );
    d3dpp.Windowed = TRUE;
    d3dpp.SwapEffect = D3DSWAPEFFECT_COPY;
    d3dpp.BackBufferFormat = D3DFMT_A8R8G8B8;
	d3dpp.BackBufferWidth = nWidth;
	d3dpp.BackBufferHeight = nHeight;
    d3dpp.EnableAutoDepthStencil = FALSE;
	d3dpp.PresentationInterval = D3DPRESENT_INTERVAL_IMMEDIATE;

    if ( FAILED( lpD3D->CreateDevice( D3DADAPTER_DEFAULT, D3DDEVTYPE_HAL, hWnd, D3DCREATE_HARDWARE_VERTEXPROCESSING, &d3dpp, &lpD3DDevice ) ) )
        return E_FAIL;

	lpD3DDevice->SetRenderState( D3DRS_ZFUNC, D3DCMP_ALWAYS );
	lpD3DDevice->SetRenderState( D3DRS_ZENABLE, FALSE );
	lpD3DDevice->SetRenderState( D3DRS_CULLMODE, D3DCULL_CW );
	lpD3DDevice->SetRenderState( D3DRS_FILLMODE, D3DFILL_SOLID );

	// création de la texture de stockage des nombres premiers

	lpD3DDevice->CreateOffscreenPlainSurface( nWidth, nHeight, D3DFMT_A8R8G8B8,
		D3DPOOL_SYSTEMMEM, &lpRenderSurface, NULL );

	// création des vertex et index buffers de la texture de stockage des nombres premiers

	D3DVERTEXELEMENT9 declaration[] = 
	{
		{ 0,  0, D3DDECLTYPE_FLOAT3, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_POSITION, 0 },
		D3DDECL_END()
	};

	lpD3DDevice->CreateVertexDeclaration( declaration, &lpVertexDeclaration );

	LPVOID pData = NULL;

	FLOAT vertices[] = { -1.0f, -1.0f, 0.0f,
						 +1.0f, -1.0f, 0.0f,
						 -1.0f, +1.0f, 0.0f,
						 +1.0f, +1.0f, 0.0f };

	if ( lpVertexBuffer ) lpVertexBuffer->Release();

	lpD3DDevice->CreateVertexBuffer( sizeof(vertices), D3DUSAGE_WRITEONLY, D3DFVF_XYZ,
		D3DPOOL_DEFAULT, &lpVertexBuffer, NULL );

	lpVertexBuffer->Lock( 0, sizeof(vertices), (LPVOID*)&pData, 0 );
	CopyMemory( pData, vertices, sizeof(vertices) );
	lpVertexBuffer->Unlock();

	WORD indices[] = { 0, 1, 2, 3, 2, 1 };

	if ( lpIndexBuffer ) lpIndexBuffer->Release();

	lpD3DDevice->CreateIndexBuffer( sizeof(indices), D3DUSAGE_WRITEONLY, D3DFMT_INDEX16, D3DPOOL_DEFAULT,
		&lpIndexBuffer, NULL );

	lpIndexBuffer->Lock( 0, sizeof(indices), (LPVOID*)&pData, 0 );
	CopyMemory( pData, indices, sizeof(indices) );
	lpIndexBuffer->Unlock();

	// création des vertex et pixel shaders de calcul des nombres premiers

	{
		LPD3DXBUFFER pCode = NULL;

		const CHAR source[] =
			"vs_3_0 \n"\
			\
			"dcl_position v0 \n"\
			"dcl_position o0 \n"\
			\
			"mov o0, v0 \n";
		D3DXAssembleShader( source, (UINT)strlen(source), NULL, NULL, NULL, &pCode, NULL );

		lpD3DDevice->CreateVertexShader( (DWORD*)pCode->GetBufferPointer(), &lpVertexShader );

		if( pCode )
			pCode->Release(); 
	}

	{
		LPD3DXBUFFER pCode = NULL;

		// c0	: aFactor[0..3]
		CHAR* pSource = new CHAR[65536];
		ZeroMemory( pSource, 65536 );

		StringCbCat( pSource, 65536, "ps_3_0 \n" );
		StringCbCat( pSource, 65536, "dcl vPos.xy \n" );
		StringCbCat( pSource, 65536, "defi i0, 1, 0, 0, 0 \n" );
		StringCbCat( pSource, 65536, "def c200, 0.0, 0.0, 0.0, 0.0 \n" );
		StringCbCat( pSource, 65536, "def c201, 1.0, 1.0, 1.0, 1.0 \n" );
		//StringCbCat( pSource, 65536, "def c202, 2.0, 2.0, 2.0, 2.0 \n" );
		//StringCbCat( pSource, 65536, "def c200, 1.0, 1.0, 1.0, 1.0 \n" );
		//StringCbCat( pSource, 65536, "def c201, -1.0, -1.0, -1.0, -1.0 \n" );
		StringCbCat( pSource, 65536, "mov r0, vPos.x \n" );
		StringCbCat( pSource, 65536, "mad r0, vPos.y, c210, r0 \n" );
		//StringCbCat( pSource, 65536, "mov r1, c202 \n" );
		//StringCbCat( pSource, 65536, "mad r0, r0, r1, c201 \n" );

		StringCbCat( pSource, 65536, "loop aL, i0 \n" );

		for ( DWORD k = 0 ; k < nConstLimit ; k++ )
		{
			CHAR sTemp[256];

			StringCbPrintf( sTemp, 256, "rcp r1.x, c%d.x \n", k );
			StringCbCat( pSource, 65536, sTemp );
			StringCbPrintf( sTemp, 256, "rcp r1.y, c%d.y \n", k );
			StringCbCat( pSource, 65536, sTemp );
			StringCbPrintf( sTemp, 256, "rcp r1.z, c%d.z \n", k );
			StringCbCat( pSource, 65536, sTemp );
			StringCbPrintf( sTemp, 256, "rcp r1.w, c%d.w \n", k );
			StringCbCat( pSource, 65536, sTemp );
			StringCbCat( pSource, 65536, "mul r2, r1, r0 \n" );
			StringCbCat( pSource, 65536, "frc r3, r2 \n" );
			StringCbCat( pSource, 65536, "sub r4, r2, r3 \n" );
			StringCbPrintf( sTemp, 256, "mul r5, r4, c%d \n", k );
			StringCbCat( pSource, 65536, sTemp );
			StringCbCat( pSource, 65536, "break_eq r5.x, r0.x \n" );
			StringCbCat( pSource, 65536, "break_eq r5.y, r0.y \n" );
			StringCbCat( pSource, 65536, "break_eq r5.z, r0.z \n" );
			StringCbCat( pSource, 65536, "break_eq r5.w, r0.w \n" );
		}

		StringCbCat( pSource, 65536, "endloop \n" );

		StringCbCat( pSource, 65536, "if_eq r5.x, r0.x \n" );
		StringCbCat( pSource, 65536, "mov r0, c200 \n" );
		StringCbCat( pSource, 65536, "mov oC0, r0 \n" );
		StringCbCat( pSource, 65536, "else \n" );
		StringCbCat( pSource, 65536, "if_eq r5.y, r0.y \n" );
		StringCbCat( pSource, 65536, "mov r0, c200 \n" );
		StringCbCat( pSource, 65536, "mov oC0, r0 \n" );
		StringCbCat( pSource, 65536, "else \n" );
		StringCbCat( pSource, 65536, "if_eq r5.z, r0.z \n" );
		StringCbCat( pSource, 65536, "mov r0, c200 \n" );
		StringCbCat( pSource, 65536, "mov oC0, r0 \n" );
		StringCbCat( pSource, 65536, "else \n" );
		StringCbCat( pSource, 65536, "if_eq r5.w, r0.w \n" );
		StringCbCat( pSource, 65536, "mov r0, c200 \n" );
		StringCbCat( pSource, 65536, "mov oC0, r0 \n" );
		StringCbCat( pSource, 65536, "else \n" );
		StringCbCat( pSource, 65536, "mov r0, c201 \n" );
		StringCbCat( pSource, 65536, "mov oC0, r0 \n" );
		StringCbCat( pSource, 65536, "endif \n" );
		StringCbCat( pSource, 65536, "endif \n" );
		StringCbCat( pSource, 65536, "endif \n" );
		StringCbCat( pSource, 65536, "endif \n" );

		size_t a = strlen(pSource);

		D3DXAssembleShader( pSource, (UINT)strlen(pSource), NULL, NULL, NULL, &pCode, NULL );

		lpD3DDevice->CreatePixelShader( (DWORD*)pCode->GetBufferPointer(), &lpPixelShader );

		if( pCode )
			pCode->Release(); 

		delete [] pSource;
	}

	return S_OK;
}




VOID Cleanup()
{
	if ( lpVertexShader )
		lpVertexShader->Release();

	if ( lpPixelShader )
		lpPixelShader->Release();

	if( lpVSCode )
		lpVSCode->Release(); 

	if( lpPSCode )
		lpPSCode->Release(); 

	if( lpVSData )
		lpVSData->Release(); 

	if( lpPSData )
		lpPSData->Release(); 

	if( lpIndexBuffer != NULL )
        lpIndexBuffer->Release();

	if( lpVertexBuffer != NULL )
		lpVertexBuffer->Release();

	if( lpVertexDeclaration != NULL )
		lpVertexDeclaration->Release();

	if( lpD3DDevice != NULL )
        lpD3DDevice->Release();

    if( lpD3D != NULL )
        lpD3D->Release();

	delete [] pFactor;
}



VOID ProcessGPU()
{
	/************************************************************************/
	/* CALCUL DES PREMIERS NOMBRES PREMIERS PAR LE CPU                      */
	/************************************************************************/

	for ( DWORD k = 0 ; k < nConstLimit * 4 ; k ++ ) pFactor[k] = 0.0f;

	nCount = 0;

	pFactor[nCount++] = 2.0f;
	pFactor[nCount++] = 3.0f;
	pFactor[nCount++] = 5.0f;

	DWORD pi = 2;

	for ( DWORD i = 7 ; i <= nRoot ; i += (pi ^= 6) )
	{
		DWORD pj = 4;

		for ( DWORD j = 5 ; (i % j) && (j * j < i) ; j += (pj ^= 6) );

		if ( i % j ) pFactor[nCount++] = (FLOAT)i;
	}

	/************************************************************************/
	/* CALCUL DES NOMBRES PREMIERS SUIVANTS PAR LE GPU                      */
	/************************************************************************/

	lpD3DDevice->Clear( 0, NULL, D3DCLEAR_TARGET, 0x00000000, 1.0f, 0 );

	if( SUCCEEDED( lpD3DDevice->BeginScene() ) )
	{
		lpD3DDevice->SetVertexShader( lpVertexShader );
		lpD3DDevice->SetPixelShader( lpPixelShader );

		lpD3DDevice->SetPixelShaderConstantF( 210, vModulo, 1 );

		lpD3DDevice->SetPixelShaderConstantF( 0, pFactor, nConstLimit );

		lpD3DDevice->SetStreamSource( 0, lpVertexBuffer, 0, 3 * sizeof(FLOAT) );

		lpD3DDevice->SetIndices( lpIndexBuffer );

		lpD3DDevice->SetVertexDeclaration( lpVertexDeclaration );

		lpD3DDevice->DrawIndexedPrimitive( D3DPT_TRIANGLELIST, 0, 0, 4, 0, 2 );

		lpD3DDevice->EndScene();
	}

	lpD3DDevice->Present( NULL, NULL, NULL, NULL );
}



VOID ProcessCPU()
{
	nCount = 0;

	pFactor[nCount++] = 2.0f;
	pFactor[nCount++] = 3.0f;
	pFactor[nCount++] = 5.0f;

	DWORD pi = 2;

	for ( DWORD i = 7 ; i <= nMax ; i += (pi ^= 6) )
	{
		DWORD pj = 4;

		for ( DWORD j = 5 ; (i % j) && (j * j < i) ; j += (pj ^= 6) );

		if ( i % j ) pFactor[nCount++] = (FLOAT)i;
	}
}



LRESULT WINAPI MsgProc( HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam )
{
    switch( msg )
    {
        case WM_DESTROY:
            PostQuitMessage( 0 );
            return 0;
    }

    return DefWindowProc( hWnd, msg, wParam, lParam );
}



//INT WINAPI WinMain( HINSTANCE hInst, HINSTANCE, LPSTR, INT )
int main()
{
	cout << "Entrer la valeur maximum de l'intervalle : ";

	cin >> nMax;
	
	bool bGPU;

	{
		cout << "Effectuer le calcul sur GPU ? (o/n) ";

		CHAR c;
		cin >> c;

		if ( c == 'o' || c == 'O' || c == 'y' || c == 'Y' ) bGPU = true; else bGPU = false;
	}

	{
		cout << "Initialisation du programme...";

		nRoot = (DWORD)sqrt( (FLOAT)nMax );

		nWidth = nRoot;
		nHeight = nRoot;

		vModulo[0] = (FLOAT)nRoot;
		vModulo[1] = (FLOAT)nRoot;
		vModulo[2] = (FLOAT)nRoot;
		vModulo[3] = (FLOAT)nRoot;

		nCount = 0;

		if ( bGPU )
			pFactor = new FLOAT[nConstLimit*4];
		else
			pFactor = new FLOAT[nMax/2];

		WNDCLASSEX wc = { sizeof(WNDCLASSEX), CS_CLASSDC, MsgProc, 0L, 0L,
			GetModuleHandle(NULL), NULL, NULL, NULL, NULL,
			"gpgpu_pn", NULL };
		RegisterClassEx( &wc );

		hInstance = wc.hInstance;

		HWND hWnd = CreateWindow( "gpgpu_pn", "GP-GPU - Nombres Premiers",
			WS_OVERLAPPEDWINDOW, 120, 80, nWidth, nHeight + 32,
			NULL, NULL, hInstance, NULL );

		Initialize( hWnd );

		ShowWindow( hWnd, SW_SHOWDEFAULT );
		UpdateWindow( hWnd );

		cout << "ok." << endl;
	}

	{
		cout << "Effectuer une mesure de performances ? (o/n) ";

		CHAR c;
		cin >> c;

		if ( c == 'o' || c == 'O' || c == 'y' || c == 'Y' )
		{
			cout << "Entrer le nombre de boucles à effectuer : ";

			DWORD dwLoop;

			cin >> dwLoop;

			cout << "Calcul des nombres premiers...";

			DWORD dwStart = GetTickCount();
			
			for ( DWORD k = 0 ; k < dwLoop ; k++ )
			{
				if ( bGPU ) ProcessGPU(); else ProcessCPU();
			}

			DWORD dwStop = GetTickCount();

			cout << "ok." << endl;

			FLOAT fTime = (FLOAT)(dwStop - dwStart) / (FLOAT)dwLoop;

			cout << "Temps de calcul moyen : " << fTime << "ms" << endl;
		}
		else
		{
			cout << "Calcul des nombres premiers...";

			if ( bGPU ) ProcessGPU(); else ProcessCPU();

			cout << "ok." << endl;
		}
	}

	{
		cout << "Copie de la mémoire vidéo à la mémoire centrale...";

		lpD3DDevice->GetRenderTarget( 0, &lpRenderTarget );

		lpD3DDevice->GetRenderTargetData( lpRenderTarget, lpRenderSurface );

		D3DLOCKED_RECT tLockedRect;

		lpRenderSurface->LockRect( &tLockedRect, NULL, D3DLOCK_DONOTWAIT );

		DWORD* pData = (DWORD*)tLockedRect.pBits;

		for ( DWORD k = 0 ; k < nCount ; k++ )
		{
			DWORD n = (DWORD)pFactor[k];

			pData[n] = 0xFFFFFFFF;
		}

		lpRenderSurface->UnlockRect();

		cout << "ok." << endl;
	}

	{
		cout << "Sauvegarder le résultat dans une image ? (o/n) ";

		CHAR c;
		cin >> c;

		if ( c == 'o' || c == 'O' || c == 'y' || c == 'Y' )
		{
			cout << "Sauvegarde du résultat...";

			D3DXSaveSurfaceToFile( "gpgpu_pn.bmp", D3DXIFF_BMP, lpRenderSurface, NULL, NULL );

			cout << "ok." << endl;
		}
	}

	{
		cout << "Afficher le résultat dans la console ? (o/n) ";

		CHAR c;
		cin >> c;

		if ( c == 'o' || c == 'O' || c == 'y' || c == 'Y' )
		{
			D3DLOCKED_RECT tLockedRect;

			lpRenderSurface->LockRect( &tLockedRect, NULL, D3DLOCK_READONLY );

			DWORD* pData = (DWORD*)tLockedRect.pBits;

			for ( DWORD k = 0 ; k < nRoot * nRoot ; k++ )
			{
				if ( pData[k] ) cout << k << endl;
			}

			lpRenderSurface->UnlockRect();
		}
	}

	system("pause");

	Cleanup();

    UnregisterClass( "gpgpu_pn", hInstance );

    return 0;
}



