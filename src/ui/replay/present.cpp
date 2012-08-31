#include "core/app.h"
#include "base/mpqfile.h"
#include "frameui/fontsys.h"

#include "script/parser.h"

#include "present.h"

#define IDC_PRESETS       130
#define IDC_SAVEPRESET    131
#define IDC_DELPRESET     132
#define IDC_EDITOR        133
#define IDC_COMPUTE       134
#define IDC_PREVIEW       135
#define IDC_COPY          136
#define IDC_RESULT        137
#define IDC_GENERATE      138

struct DefPreset
{
  char name[256];
  char path[256];
};
static DefPreset defPresets[] = {
  {"Full info (PlayDota BB-code)", "dota\\ppreset1.txt"},
  {"Compact (PlayDota BB-code)", "dota\\ppreset2.txt"},
  {"New...", ""}
};
static const int numDefPresets = sizeof defPresets / sizeof defPresets[0];

ReplayPresentTab::ReplayPresentTab(Frame* parent)
  : ReplayTab(parent)
{
  StaticFrame* tip = new StaticFrame("Preset:", this);
  tip->setPoint(PT_TOPLEFT, 10, 13);
  tip->setHeight(18);
  presets = new ComboFrame(this, IDC_PRESETS, CBS_DROPDOWN);
  savePreset = new ButtonFrame("Save", this, IDC_SAVEPRESET);
  delPreset = new ButtonFrame("Delete", this, IDC_DELPRESET);
  StaticFrame* vline = new StaticFrame(this, 0, SS_ETCHEDVERT);
  ButtonFrame* generate = new ButtonFrame("Generate...", this, IDC_GENERATE);

  generate->setSize(80, 21);
  generate->setPoint(PT_TOPRIGHT, -10, 10);
  vline->setSize(2, 23);
  vline->setPoint(PT_TOPRIGHT, generate, PT_TOPLEFT, -4, 0);

  presets->setPoint(PT_BOTTOMLEFT, tip, PT_BOTTOMRIGHT, 5, 0);
  savePreset->setSize(60, 21);
  delPreset->setSize(60, 21);
  delPreset->setPoint(PT_TOPRIGHT, vline, PT_TOPLEFT, -4, 0);
  savePreset->setPoint(PT_TOPRIGHT, delPreset, PT_TOPLEFT, -5, 0);
  presets->setPoint(PT_BOTTOMRIGHT, savePreset, PT_BOTTOMLEFT, -5, 0);

  editor = new ScriptEditor(this);
  editor->setPoint(PT_TOPLEFT, tip, PT_BOTTOMLEFT, 0, 5);
  editor->setPointEx(PT_BOTTOMRIGHT, 1, 0.7, -10, -18);

  ButtonFrame* compute = new ButtonFrame("Compute", this, IDC_COMPUTE);
  //ButtonFrame* preview = new ButtonFrame("Preview", this, IDC_PREVIEW);
  ButtonFrame* copy = new ButtonFrame("Copy", this, IDC_COPY);
  compute->setHeight(21);
  //preview->setHeight(21);
  copy->setHeight(21);
  compute->setPoint(PT_TOPLEFT, editor, PT_BOTTOMLEFT, 0, 5);
  copy->setPoint(PT_TOPRIGHT, editor, PT_BOTTOMRIGHT, 0, 5);
  compute->setPointEx(PT_RIGHT, 0.5, 0, -2, 0);
  copy->setPoint(PT_LEFT, compute, PT_RIGHT, 5, 0);
  //preview->setPoint(PT_TOPLEFT, compute, PT_TOPRIGHT, 5, 0);
  //preview->setPoint(PT_TOPRIGHT, copy, PT_TOPLEFT, -5, 0);

  result = new EditFrame(this, 0, ES_MULTILINE | ES_READONLY | WS_HSCROLL | WS_VSCROLL);
  result->setPoint(PT_TOPLEFT, compute, PT_BOTTOMLEFT, 0, 5);
  result->setPoint(PT_BOTTOMRIGHT, -10, -10);
  result->setBgColor(0xFFFFFF);
  result->setFont(FontSys::getFont(12, "Courier New", 0));

  for (int i = 0; i < numDefPresets; i++)
    presets->addString(defPresets[i].name, -i - 1);
  for (int i = 0; i < cfg.fmtPresets.length(); i++)
    presets->addString(cfg.fmtPresets[i], i);
  editor->setText(cfg.fmtScript);

  updateValidity();
}

void ReplayPresentTab::onSetReplay()
{
  result->setText("");
}
void ReplayPresentTab::updateValidity()
{
  String name = presets->getText();
  bool valid = !name.isEmpty();
  for (int i = 0; i < numDefPresets && valid; i++)
    if (!name.icompare(defPresets[i].name))
      valid = false;
  savePreset->enable(valid);
  valid = false;
  for (int i = 0; i < cfg.fmtPresets.length() && !valid; i++)
    if (!name.icompare(cfg.fmtPresets[i]))
      valid = true;
  delPreset->enable(valid);
}
uint32 ReplayPresentTab::onMessage(uint32 message, uint32 wParam, uint32 lParam)
{
  if (message == WM_COMMAND)
  {
    switch (LOWORD(wParam))
    {
    case IDC_PRESETS:
      if (HIWORD(wParam) == CBN_EDITCHANGE)
        updateValidity();
      else if (HIWORD(wParam) == CBN_SELCHANGE)
      {
        int sel = presets->getCurSel();
        if (sel != CB_ERR)
        {
          if (editor->modified())
          {
            if (MessageBox(getApp()->getMainWindow(),
                "This will discard the changes made to the current preset.", "Warning",
                MB_ICONWARNING | MB_OKCANCEL) != IDOK)
            {
              presets->setCurSel(-1);
              break;
            }
          }
          int id = presets->getItemData(sel);
          if (id < 0)
          {
            if (defPresets[-id - 1].path[0] == 0)
            {
              editor->setText("");
              presets->setCurSel(-1);
            }
            else
            {
              File* file = getApp()->getResources()->openFile(defPresets[-id - 1].path, File::READ);
              if (file)
              {
                editor->setText(file->gets(true));
                delete file;
              }
            }
            savePreset->enable(false);
            delPreset->enable(false);
          }
          else
          {
            File* file = File::open(String::buildFullName(getApp()->getRootPath(),
              String::format("preset%d.txt", id + 1)), File::READ);
            if (file)
            {
              editor->setText(file->gets(true));
              delete file;
            }
            savePreset->enable(true);
            delPreset->enable(true);
          }
        }
      }
      break;
    case IDC_SAVEPRESET:
      {
        String name = presets->getText();
        for (int i = 0; i < numDefPresets; i++)
          if (!name.icompare(defPresets[i].name))
            return 0;
        editor->setModified(false);
        for (int i = 0; i < cfg.fmtPresets.length(); i++)
        {
          if (!name.icompare(cfg.fmtPresets[i]))
          {
            cfg.fmtPresets[i] = name;
            String text = editor->getText();
            File* file = File::open(String::buildFullName(getApp()->getRootPath(),
              String::format("preset%d.txt", i + 1)), File::REWRITE);
            if (file)
            {
              file->write(text.c_str(), text.length());
              delete file;
            }
            return 0;
          }
        }
        int id = cfg.fmtPresets.length();
        presets->setCurSel(presets->addString(name, id));
        cfg.fmtPresets.push(name);
        String text = editor->getText();
        File* file = File::open(String::buildFullName(getApp()->getRootPath(),
          String::format("preset%d.txt", id + 1)), File::REWRITE);
        if (file)
        {
          file->write(text.c_str(), text.length());
          delete file;
        }
        updateValidity();
      }
      break;
    case IDC_DELPRESET:
      {
        String name = presets->getText();
        for (int i = 0; i < cfg.fmtPresets.length(); i++)
        {
          if (!name.icompare(cfg.fmtPresets[i]))
          {
            if (MessageBox(getApp()->getMainWindow(),
                String::format("Are you sure you want to delete preset \"%s\"?", cfg.fmtPresets[i]),
                "Warning", MB_YESNO | MB_ICONWARNING) != IDYES)
              return 0;
            DeleteFile(String::buildFullName(getApp()->getRootPath(),
              String::format("preset%d.txt", i + 1)));
            for (int j = i + 1; j < cfg.fmtPresets.length(); j++)
            {
              MoveFile(String::buildFullName(getApp()->getRootPath(),
                  String::format("preset%d.txt", j + 1)),
                String::buildFullName(getApp()->getRootPath(),
                  String::format("preset%d.txt", j)));
              cfg.fmtPresets[j - 1] = cfg.fmtPresets[j];
            }
            presets->delString(i + numDefPresets);
            presets->setCurSel(-1);
            updateValidity();
            break;
          }
        }
      }
      break;
    case IDC_COMPUTE:
      if (w3g)
      {
        ScriptParser* parser = new ScriptParser(editor->getText());
        result->setText(parser->run(w3g));
        int line, col;
        if (parser->getError(line, col))
        {
          editor->setCursor(line, col);
          SetFocus(editor->getHandle());
        }
        delete parser;
      }
      break;
    case IDC_COPY:
      {
        String text = result->getText();
        File* f = File::open("test.txt", File::REWRITE);
        f->write(text.c_str(), text.length());
        delete f;
        SetClipboard(CF_TEXT, CreateGlobalText(result->getText()));
      }
      break;
    case IDC_GENERATE:
      {
        String result;
        if (DialogBoxParam(getApp()->getInstanceHandle(), MAKEINTRESOURCE(IDD_SCRIPTGEN),
            editor->getHandle(), ScriptGenDlgProc, (LPARAM) &result) == IDOK)
        {
          editor->setText(result);
          editor->setModified(true);
        }
      }
      break;
    }
    return 0;
  }
  else if (message == FM_SHUTDOWN)
  {
    cfg.fmtScript = editor->getText();
    return 0;
  }
  return M_UNHANDLED;
}
