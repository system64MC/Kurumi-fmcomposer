#include "popup.hpp"
#include "../../streamed/streamedExport.h"
#include "../../views/pattern/songEditor.hpp"
#include "portaudio.h"


#include "../../libs/tinyfiledialogs/tinyfiledialogs.h"
#include "../../views/instrument/instrEditor.hpp"
#include "../../views/pattern/songFileActions.hpp"



extern InstrEditor *instrEditor;
extern SongEditor* songEditor;


int exporting = 0;

extern string exportFileName;

extern PaStream *stream;
extern PaStreamParameters out;
static char fxList[26] = { 'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H', 'I', 'J', 'K', 'M', 'N', 'P', 'Q', 'R', 'S', 'T', 'V', 'W', 'X', 'Y' };



void Popup::buttonActions(int buttonID)
{
	switch (type)
	{
		case POPUP_MULTITRACKEXPORT:
			switch (buttonID)
			{
				case 0:
					lists[0].add("#"+std::to_string(lists[0].text.size()+1));
					streamedExport.multitrackAssoc.resize(streamedExport.multitrackAssoc.size()+1);
					streamedExport.multitrackAssoc[streamedExport.multitrackAssoc.size() - 1].resize(1);
					streamedExport.multitrackAssoc[streamedExport.multitrackAssoc.size() - 1][0]=0;
					lists[0].select(lists[0].text.size()-1);
					updateMultitrackExportList();
					break;
				case 1:
					if (streamedExport.multitrackAssoc.size() > 1) {
						streamedExport.multitrackAssoc.erase(streamedExport.multitrackAssoc.begin()+lists[0].value);
						lists[0].remove(lists[0].value);
						for (unsigned i = 0; i < lists[0].text.size(); i++)
						{
							lists[0].text[i].setString("#"+std::to_string(i+1));
						}
						updateMultitrackExportList();
					}
					break;
				case 2:
					for (unsigned i = 0; i < FM_ch; i++)
					{
						streamedExport.mutedChannels[i] = fm->ch[i].muted;
					}
					streamedExport.multiTrackIter=0;
					promptStreamedExport();
					break;
				case 3:
					close();
					break;



			}
			break;
		case POPUP_WRONGVERSION:
			if (buttonID == 0)
				close();
			else if (buttonID == 1)
#ifdef _WIN32
				ShellExecute(0, 0, "http://fmcomposer.org/download.php?version=latest", 0, 0, SW_SHOW);
#elif __linux__ || __APPLE__
				system("xdg-open http://fmcomposer.org/download.php?version=latest &");
#endif
			break;
		case POPUP_FIRSTSTART:
			if (buttonID == 0)
				close();
			else if (buttonID == 1)
#ifdef _WIN32
				ShellExecute(0, 0, "http://fmcomposer.org/user_manual.php#tutorial", 0, 0, SW_SHOW);
#elif __linux__ || __APPLE__
				system("xdg-open http://fmcomposer.org/user_manual.php#tutorial &");
#endif
			break;
		case POPUP_ABOUT:
			if (buttonID == 0)
				close();
			else if (buttonID == 1)
#ifdef _WIN32
				ShellExecute(0, 0, "http://fmcomposer.org", 0, 0, SW_SHOW);
#elif __linux__ || __APPLE__
				system("xdg-open http://fmcomposer.org &");
#endif
			else if (buttonID == 2)
#ifdef _WIN32
				ShellExecute(0, 0, string(string("http://fmcomposer.org/checkupdates.php?v=") + VERSION).c_str(), 0, 0, SW_SHOW);
#elif __linux__ || __APPLE__
				system(string(("xdg-open "+string("http://fmcomposer.org/checkupdates.php?v=")+VERSION)+" &").c_str());
#endif
			break;
		case POPUP_WORKING:
			if (buttonID == 0)
			{ // replace button
				stopExport();
			}
			close();
			break;
		case POPUP_REPLACE_INSTRUMENT:
			if (buttonID == 0)
			{ // replace button
				songEditor->multipleEdit(8, NULL, lists[0].value);
			}
			close();
			break;
		case POPUP_FADE:
			if (buttonID == 0)
			{ // ok button
				if (checkboxes[0].checked)
					songEditor->multipleEdit(11, NULL, sliders[2].value);
				else if (checkboxes[1].checked)
					songEditor->multipleEdit(9, NULL, sliders[0].value, sliders[1].value);
				else
					songEditor->multipleEdit(12, NULL, sliders[3].value, sliders[1].value);
			}
			close();
			break;
		case POPUP_SEARCH:
			if (buttonID == 0)
			{ // ok button
				int searchWhat = checkboxes[0].checked + checkboxes[1].checked * 2 + checkboxes[2].checked * 4 + checkboxes[3].checked * 8 + checkboxes[4].checked * 16;

				int searchIn = checkboxes[6].checked + checkboxes[7].checked * 2;

				int replaceWhat = (checkboxes[9].checked + checkboxes[10].checked * 2 + checkboxes[11].checked * 4 + checkboxes[12].checked * 8 + checkboxes[13].checked * 16 + checkboxes[14].checked * 32) * checkboxes[8].checked;

				unsigned char searchValues[5] = {
					sliders[0].value,
					sliders[1].value,
					lists[0].value - 1,
					lists[1].value - 1,
					sliders[2].value
				};

				if (lists[1].value > 0)
				{
					searchValues[3] = fxList[lists[1].value - 1];
				}

				unsigned char replaceValues[6] = {
					sliders[3].value,
					sliders[4].value,
					lists[2].value - 1,
					lists[3].value - 1,
					sliders[5].value,
					sliders[6].value,
				};

				if (lists[3].value > 0)
				{
					replaceValues[3] = fxList[lists[3].value - 1];
				}


				int result = songEditor->search(searchWhat, searchValues, searchIn, replaceWhat, replaceValues);

				if (replaceWhat)
				{
					show(POPUP_REPLACERESULT, result);
				}
				else
				{
					close();
				}
			}
			else if (buttonID == 1)
			{
				close();
			}

			break;
		case POPUP_SAVEFAILED:
		case POPUP_OPENFAILED:
			close();
			break;
		case POPUP_FILECORRUPTED:
			close();
			break;
		case POPUP_EFFECTS:
			if (buttonID == 0)
			{ // ok button

				int fxValue;

				if (lists[0].value == 7 || lists[0].value == 9)
				{
					fxValue = sliders[0].value * 16 + sliders[1].value;
				}
				else
				{
					fxValue = sliders[0].value;
				}

				songEditor->multipleEdit(10, NULL, fxList[lists[0].value], fxValue);

			}
			close();
			break;
		case POPUP_QUITCONFIRM:
			switch (buttonID)
			{
				case 0: // quit button
					window->close();
					break;
				case 1: // cancel
					close();
					break;
				case 2: // save and quit
					if (song_save())
					{
						window->close();
						close();
					}
					break;
			}
			break;
		case POPUP_OPENCONFIRM:
			switch (buttonID)
			{
				case 0: // quit button
					isSongModified=0;
					song_load(songLoadedRequest.c_str(), false);
					close();
					break;
				case 1: // cancel
					close();
					break;
				case 2: // save
					if (song_save())
					{
						song_load(songLoadedRequest.c_str(), false);
						close();
					}
					break;
			}
			break;
		case POPUP_DELETEINSTRUMENT:
			if (buttonID == 0)
			{ // yes button
				instrEditor->removeInstrument();
				for (unsigned ch = 0; ch < FM_ch; ++ch)
					songEditor->updateChannelData(ch);
			}
			close();
			break;
		case POPUP_NEWFILE:
			switch (buttonID)
			{
				case 0: // quit button
					isSongModified=0;
					song_clear();
					close();
					break;
				case 1: // cancel
					close();
					break;
				case 2: // save
					song_save();
					song_clear();
					close();
					break;
			}

			close();
			break;
		case POPUP_MIDIEXPORT:

			if (buttonID == 0)
			{ // ok button
				mouse.clickLock2 = 1;
				static const char * filters[1] = { "*.mid" };
				const char *fileName = tinyfd_saveFileDialog("Export as MIDI", NULL, 1, filters, "MIDI file");
				if (fileName)
				{
					string fileNameOk = forceExtension(fileName, "mid");
					midiExport(fileNameOk.c_str());
					popup->show(POPUP_SAVED);
					close();
				}
			}
			if (buttonID == 1)
			{ // cancel button
				close();
			}

			break;

		case POPUP_STREAMEDEXPORT:

			if (buttonID == 0)
			{ // ok button
				streamedExport.fromPattern = sliders[1].value;
				streamedExport.toPattern = sliders[2].value;
				streamedExport.nbLoops = sliders[3].value;
				streamedExport.format = checkboxes[0].checked+2*checkboxes[1].checked;
				mouse.clickLock2 = 1;

				
				// WAVE
				if (checkboxes[0].checked)
				{
					streamedExport.bitDepth = sliders[5].value;
				}
				// MP3
				else if (checkboxes[1].checked)
				{
					streamedExport.param = (sliders[0].value) + checkboxes[3].checked * 100;
				}
				// FLAC
				else
				{
					streamedExport.bitDepth = sliders[6].value;
					streamedExport.param = sliders[4].value;
				}

				// multi-track export
				if (checkboxes[5].checked) {
					show(POPUP_MULTITRACKEXPORT);
				}
				// single-track export
				else
				{
					streamedExport.multitrackAssoc.clear();
					promptStreamedExport();
				}


				
			}
			if (buttonID == 1)
			{ // cancel button
				close();
			}

			break;

		case POPUP_TRANSPOSE:
			if (buttonID == 0)
			{ // ok button
				if (checkboxes[0].checked)
					songEditor->multipleEdit(4, NULL, sliders[0].value);
				else
					songEditor->multipleEdit(3, NULL, sliders[1].value);

			}
			close();
			break;
		case POPUP_SETNOTE:
			if (buttonID == 0)
			{ // ok button
				songEditor->multipleEdit(4, NULL, sliders[0].value);
			}
			close();
			break;
		case POPUP_TEMPERAMENT:

			if (buttonID == 1)
			{
				for (int i = 0; i < 12; i++)
					sliders[i].setValue(0);
			}
			if (buttonID == 0 )
				close();
			break;
		case POPUP_INSERTROWS:
			if (buttonID == 0)
			{
				songEditor->pattern_insertrows(sliders[0].value);
			}
			close();
			break;
		case POPUP_REMOVEROWS:
			if (buttonID == 0)
			{
				songEditor->pattern_deleterows(sliders[0].value);
			}
			close();
			break;
	}
}
